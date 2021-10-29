#include <iostream>
#include <sstream>
#include <mutex>

#define BILLION  1000000000L

using namespace std;
pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t max_mutex=PTHREAD_MUTEX_INITIALIZER;

template<class K,class V,int MAXLEVEL>
class skiplist_node
{
public:
    skiplist_node()
    {
        for ( int i=1; i<=MAXLEVEL; i++ ) {
            forwards[i] = NULL;
        }
        locked = false;
    }
 
    skiplist_node(K searchKey):key(searchKey)
    {
        for ( int i=1; i<=MAXLEVEL; i++ ) {
            forwards[i] = NULL;
        }
        locked = false;
    }
 
    skiplist_node(K searchKey,V val):key(searchKey),value(val)
    {
        for ( int i=1; i<=MAXLEVEL; i++ ) {
            forwards[i] = NULL;
        }
        locked = false;
    }
    
    void lock(){
        node_lock.lock();
        locked = true;
    }
    void unlock(){
        node_lock.unlock();
        locked = false;
    }

    virtual ~skiplist_node()
    {
    }
 
    K key;
    V value;
    mutex node_lock;
    bool locked;
    skiplist_node<K,V,MAXLEVEL>* forwards[MAXLEVEL+1];
};
 
///////////////////////////////////////////////////////////////////////////////
 
template<class K, class V, int MAXLEVEL = 16>
class skiplist
{
public:
    typedef K KeyType;
    typedef V ValueType;
    typedef skiplist_node<K,V,MAXLEVEL> NodeType;
 
    skiplist(K minKey,K maxKey):m_pHeader(NULL),m_pTail(NULL),
                                max_curr_level(1),max_level(MAXLEVEL),
                                m_minKey(minKey),m_maxKey(maxKey)
    {
        m_pHeader = new NodeType(m_minKey);
        m_pTail = new NodeType(m_maxKey);
        for ( int i=1; i<=MAXLEVEL; i++ ) {
            m_pHeader->forwards[i] = m_pTail;
        }
        pthread_rwlock_init(&rw, NULL);
    }
 
    virtual ~skiplist()
    {
        NodeType* currNode = m_pHeader->forwards[1];
        while ( currNode != m_pTail ) {
            NodeType* tempNode = currNode;
            currNode = currNode->forwards[1];
            delete tempNode;
        }
        delete m_pHeader;
        delete m_pTail;
        pthread_rwlock_destroy(&rw);
    }
 
    void insert(K searchKey,V newValue)
    {
      //  fprintf(stderr, "2, %d, %d, %d \n", searchKey, newValue, max_curr_level);
        skiplist_node<K,V,MAXLEVEL>* update[MAXLEVEL];
    
        NodeType* currNode = m_pHeader;
        NodeType* prevNode;
        // 직전 정보들을 update에 저장
        bool flag, headerFlag = false, tailFlag = false;
        int localMax;
        pthread_rwlock_wrlock(&rw);
        localMax = max_curr_level;
        pthread_rwlock_unlock(&rw);
        prevNode = NULL;
        for(int level=localMax; level >=1; level--) {
            while ( currNode->forwards[level]->key < searchKey ) {
                currNode = currNode->forwards[level];
            }
            if(prevNode!=currNode){

          // fprintf(stderr, "1add%p search %d\n", currNode, searchKey);
                if(currNode == m_pHeader)
                headerFlag = true;
                currNode->lock();
            }
            if(prevNode!=NULL){
                if((prevNode -> forwards[level+1] != currNode->forwards[level])){
                    
          //  fprintf(stderr, "2add%p \n", currNode->forwards[level]);
                    if(currNode->forwards[level] == m_pTail)
                        tailFlag = true;
                    currNode->forwards[level]->lock();
                }
            }
            else   {

         //   fprintf(stderr, "3add%p \n", currNode->forwards[level]);
            if(currNode->forwards[level] == m_pTail)
                        tailFlag = true;
                currNode->forwards[level]->lock();
            }

            prevNode = currNode;
            update[level] = currNode;
        }
        
        currNode = currNode->forwards[1];
        K key = currNode->key;
        if ( key == searchKey ) {
            // duplicate insertion: 새로운 value로 update
            currNode->value = newValue;
        }
        else {
            // 새로운 노드의 level을 랜덤하게 배정
            int newlevel = randomLevel();
            if ( newlevel > localMax ) {
                //새로운 최고 높이라면 맨 위부터는 헤더가 직전 노드
                flag = true;
                if(!headerFlag)
                    m_pHeader -> lock();
                if(!tailFlag)
                    m_pTail -> lock();
                for ( int level = localMax+1; level <= newlevel; level++ ) {
                    //여기도 락 걸어줘야 할수도
                    update[level] = m_pHeader;
                }
                
                pthread_rwlock_wrlock(&rw);
                max_curr_level = newlevel;
                pthread_rwlock_unlock(&rw);

            }
            currNode = new NodeType(searchKey,newValue);
            
            // 이거 max curr level에서 newlevel로 바꿈. 아무리봐도 newlevel인데 ..
            for ( int lv=1; lv<=localMax; lv++ ) {
                currNode->forwards[lv] = update[lv]->forwards[lv];
                update[lv]->forwards[lv] = currNode;
            }
        }

     //  pthread_rwlock_unlock(&rw);
            prevNode = NULL;
            for(int i = 1; i<=localMax;i++){
                if(prevNode != update[i]){
                  //  fprintf(stderr, "uladd%p %p \n", update[i], currNode->forwards[i]);
                    update[i]->unlock();
                    if(i==1)
                        currNode->forwards[i] -> unlock();
                }
                if(i>1)
                if(currNode -> forwards[i-1]!=currNode->forwards[i]){
                    currNode->forwards[i] -> unlock();
                }
                prevNode = update[i];
            }
            if(!headerFlag)
            m_pHeader->unlock();
            if(!tailFlag)
            m_pTail -> unlock();
         //  fprintf(stderr, "4, %d, %d\n", searchKey, max_curr_level);
    }
 
    void erase(K searchKey)
    {
        skiplist_node<K,V,MAXLEVEL>* update[MAXLEVEL];
        NodeType* currNode = m_pHeader;
        for(int level=max_curr_level; level >=1; level--) {
            while ( currNode->forwards[level]->key < searchKey ) {
                currNode = currNode->forwards[level];
            }
            update[level] = currNode;
        }
        currNode = currNode->forwards[1];
        if ( currNode->key == searchKey ) {
            for ( int lv = 1; lv <= max_curr_level; lv++ ) {
                if ( update[lv]->forwards[lv] != currNode ) {
                    break;
                }
                update[lv]->forwards[lv] = currNode->forwards[lv];
            }
            delete currNode;
            // update the max level
            while ( max_curr_level > 1 && m_pHeader->forwards[max_curr_level] == NULL ) {
                max_curr_level--;
            }
        }
    }
 
    //const NodeType* find(K searchKey)
    V find(K searchKey)
    {
        NodeType* currNode = m_pHeader;
        for(int level=max_curr_level; level >=1; level--) {
            while ( currNode->forwards[level]->key < searchKey ) {
                currNode = currNode->forwards[level];
            }
        }
        currNode = currNode->forwards[1];
        if ( currNode->key == searchKey ) {
            return currNode->value;
        }
        else {
            //return NULL;
            return -1;
        }
    }
 
    bool empty() const
    {
        return ( m_pHeader->forwards[1] == m_pTail );
    }
 
    std::string printList()
    // 얘는 어쩔수 없이 통째로 락을 걸어야 겠다 ..
    {
	int i=0;
        std::stringstream sstr;
        NodeType* currNode = m_pHeader->forwards[1];
        while ( currNode != m_pTail ) {
            //sstr << "(" << currNode->key << "," << currNode->value << ")" << endl;
            sstr << currNode->key << " ";
            currNode = currNode->forwards[1];
	    i++;
	    if(i>200) break;
        }
        return sstr.str();
    }
    
    void lock(){
        list_lock.lock();
    }

    void unlock(){
        list_lock.unlock();
    }


    const int max_level;
 
protected:
    double uniformRandom()
    {
        return rand() / double(RAND_MAX);
    }
 
    int randomLevel() {
        int level = 1;
        double p = 0.5;
        while ( uniformRandom() < p && level < MAXLEVEL ) {
            level++;
        }
        return level;
    }
    K m_minKey;
    K m_maxKey;
    int max_curr_level;
    mutex list_lock;
    skiplist_node<K,V,MAXLEVEL>* m_pHeader;
    skiplist_node<K,V,MAXLEVEL>* m_pTail;
};
 
///////////////////////////////////////////////////////////////////////////////
 
