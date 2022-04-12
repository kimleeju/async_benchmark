#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <vector>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <errno.h>
#include <thread>
#include <random>
#include <map>
#include <boost/algorithm/string.hpp>
#define TH_MAX 40
using namespace std;
using boost::asio::ip::tcp;

template<typename T>

class a_queue
{
public:
    void push( const T& value )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queque.push(value);
    }

    T pop()
    {
        if(m_queque.empty())
            return -1;
        T temp = m_queque.front();
        m_queque.pop();
        return temp;
    }

public:
    std::queue<T> m_queque;
    mutable std::mutex m_mutex;
};

a_queue<int> th_queue;
bool b_finish;

vector<int> cnt(TH_MAX,0);
queue<string> total_key;
queue<string> th_key[TH_MAX];        

a_queue<long long> set_latency;
a_queue<long long> get_latency;


static long long ustime(void){
    struct timeval tv;
    long long ust;
    gettimeofday(&tv, NULL);
    ust= ((long)tv.tv_sec)*1000000;
    ust+=tv.tv_usec;
    return ust;
}


string gen_random(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}


void create_request(const char* IP, const char* PORT, bool set_flag, int k_size, int v_size, int th_num, pthread_cond_t *cond, pthread_mutex_t* mutex, queue<string> *k_que) {
    tcp::iostream stream(IP, PORT);
    string key, value, msg, recv;
    char *xvalue;
    long long start;
    if(!stream)
    {
        cout<<"No address. Unable to connect " <<stream.error().message() << endl;
        return;
    }
    //stream << "Hello";
    //stream << endl;
    while(1){
        th_queue.push(th_num);
        
        pthread_cond_wait(&cond[th_num], &mutex[th_num]);

        if(b_finish == true)
            return;
        if(set_flag == true){
            key=gen_random(k_size);

            xvalue = (char*)malloc(v_size*sizeof(char));

            memset(xvalue, 'x', v_size); 
            value = xvalue;

            msg ="*3\r\n$3\r\nSET\r\n";
            
            msg+="$";
            msg+=(to_string(k_size));
            msg+=("\r\n");
            msg+=(key);
            msg+=("\r\n");

            msg+=("$");
            msg+=(to_string(v_size));
            msg+=("\r\n");
            msg+=(value);
            msg+=("\r\n");

            start = ustime();
            stream << msg ;
            set_latency.push(ustime() - start);
            
            getline(stream, recv);
           // if(recv.find("+OK") != string::npos){
            k_que[th_num].push(key);
            cnt[th_num]++;
           // }
            free(xvalue);
        }

        if(set_flag == false){
            key=th_key[th_num].front();
            th_key[th_num].pop();
            msg ="*2\r\n$3\r\nGET\r\n";
            msg+=("$");
            msg+=(to_string(key.size()));
            msg+=("\r\n");
            msg+=(key);
            msg+=("\r\n");
            
            start = ustime();
            stream << msg ;
            get_latency.push(ustime() - start);
            

            getline(stream, recv); // value size
            getline(stream, recv); // value
            cnt[th_num]++;
        }
    }
}
int main(int argc, char* argv[]){
    try{

        if(argc != 7){
            cerr << "Usage : <host> <port> <operation number> <key_size> <value_size> <lambda>" << endl;
            return 1;
        }

        unsigned int size;

        long long average=0;
//        c.create_conn(argv[1], argv[2]);
        
		int count=0;
		int operation_count = atoi(argv[3]);
        

        mt19937 gen(operation_count);
        poisson_distribution<> d(atoi(argv[6]));

        bool th_flag=true;
        b_finish = false;        

        if(atoi(argv[5]) == 0){
           
            th_flag=false;
        }
        thread tid[TH_MAX];
        pthread_cond_t cond[TH_MAX];
        pthread_mutex_t mutex[TH_MAX];
        queue<string> k_que[TH_MAX];

        vector<long long> set_result;
        vector<long long> get_result;

        int i, th_num;
        int total_operation ;

        for(i = 0; i < TH_MAX ; i++){
            pthread_cond_init(&cond[i],NULL);
            tid[i] = thread(create_request,argv[1],argv[2], true, atoi(argv[4]), atoi(argv[5]), i , cond, mutex, k_que);
        }
        
        for(i = 0 ; i < operation_count; i++){
            usleep(d(gen));
            th_num = th_queue.pop();
            
            if(th_num == -1){
                usleep(1000);
                i--;
                continue;
            }
            pthread_cond_signal(&cond[th_num]);
            cout<<"...... SET Operation is working : " << (double)i/operation_count * 100 <<"% ......\r";
        }
        cout<<endl;
       while(1){
            for(i = 0 ; i < TH_MAX; i++){
                total_operation += cnt[i];
            }
            if(total_operation == operation_count){
                b_finish = true;
                break;
            }
            else{
                total_operation = 0;
                sleep(1);
                continue;
            }
        }
        
        for(i = 0; i < TH_MAX ; i++){
            pthread_cond_signal(&cond[i]);
        }

        for(i = 0; i < TH_MAX ; i++){
            tid[i].join();
        }

        for(i = 0 ; i < operation_count ; i++){
            set_result.push_back(set_latency.m_queque.front());
            set_latency.pop();
        }

        sort(set_result.begin(), set_result.end());
        long long set_avg = 0;

        for(auto &r : set_result)
            set_avg += r;
        cout<<"[SET] IOPS : " << ((long long)operation_count)/(set_avg/set_result.size()) << endl;        
        cout<<"[SET] Avarage Latency : "<<set_avg/set_result.size() << endl;
        cout<<"[SET] 95% Latency : "<<set_result[set_result.size()*0.95] << endl;
        cout<<"[SET] 99% Latency : "<<set_result[set_result.size()*0.99] << endl;
        cout<<"[SET] 99.9% Latency : "<<set_result[set_result.size()*0.999] << endl;
        cout<<"[SET] 99.99% Latency : "<<set_result[set_result.size()*0.9999] << endl;
        
        cout<<"=========================================" << endl;
        //GET
        b_finish = false;
        total_operation = 0;
        while(!th_queue.m_queque.empty())th_queue.pop();
        
        for(auto &c : cnt)
            c=0;
#if 1
        for(i = 0 ; i<TH_MAX ; i++){
            while(!k_que[i].empty()){
                total_key.push(k_que[i].front());
                k_que[i].pop();
            }
        }
#endif
        for(i = 0; i < TH_MAX ; i++){
            pthread_cond_init(&cond[i],NULL);
            tid[i] = thread(create_request,argv[1],argv[2], false, atoi(argv[4]), atoi(argv[5]), i , cond, mutex, k_que);
        }
    

        for(i = 0 ; i < operation_count; i++){ 
            usleep(d(gen));
            th_num = th_queue.pop();
#if 1
            if(th_num == -1){
                usleep(1000);
                i--;
                continue;
            }
#endif
            th_key[th_num].push(total_key.front());
            total_key.pop();
            pthread_cond_signal(&cond[th_num]);
        
            cout<<"...... GET Operation is working : " <<(double)i/operation_count * 100 <<"%\r ......";
        }
        cout<<endl;
        while(1){
            for(i = 0 ; i < TH_MAX; i++){
                total_operation += cnt[i];
            }
            if(total_operation == operation_count){
                b_finish = true;
                break;
            }
            else{
                total_operation = 0;
                sleep(1);
                continue;
            }
        }
        
        for(i = 0; i < TH_MAX ; i++){
            pthread_cond_signal(&cond[i]);
        }

        for(i = 0; i < TH_MAX ; i++){
            tid[i].join();
        }

        for(i = 0 ; i < operation_count ; i++){
            get_result.push_back(get_latency.m_queque.front());
            get_latency.pop();
        }

        sort(get_result.begin(), get_result.end());
        long long get_avg = 0;

        for(auto &r : get_result)
            get_avg += r;

        cout<<"[GET] IOPS : " << ((long long)operation_count)/(get_avg/get_result.size()) << endl;        
        cout<<"[GET] Avarage Latency : "<<get_avg/get_result.size()<<endl;
        cout<<"[GET] 95% Latency : "<<get_result[get_result.size()*0.95]<<endl;
        cout<<"[GET] 99% Latency : "<<get_result[get_result.size()*0.99]<<endl;
        cout<<"[GET] 99.9% Latency : "<<get_result[get_result.size()*0.999]<<endl;
        cout<<"[GET] 99.99% Latency : "<<get_result[get_result.size()*0.9999]<<endl;
    
        cout<<"========================================="<<endl;
    }catch(exception& e){
        cerr<<"Exception : " << e.what() << endl;
    }
    return 0;
}

