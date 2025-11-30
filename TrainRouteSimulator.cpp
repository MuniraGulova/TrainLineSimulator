#include<iostream>
#include<string>
#include<chrono>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <utility>

std::string now() {
    time_t t = time(nullptr); 
    tm *lt = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", lt);
    return buf;
}

struct Line {
    std::string from;
    std::string to;
    int travelTime;

    Line(std::string f, std::string t, int time): from(std::move(f)), to(std::move(t)), travelTime(time) {
    }
};

std::map<std::string, std::mutex> stationLocks;
std::mutex coutMutex;

void trainThread(int id, const std::vector<Line> &route) { {
        std::lock_guard<std::mutex> lg(coutMutex);
        std::cout << "[" << now() << "] Train " << id << " STARTED: " << route.front().from << "->" << route.back().to <<
                std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (const Line &seg: route) {
        {
            std::lock_guard<std::mutex> lg(coutMutex);
            std::cout << " [" << now() << "] Train " << id << " travelling " << seg.from << " -> " << seg.to <<
                    std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(seg.travelTime));
        std::mutex &stLock = stationLocks[seg.to];
        stLock.lock(); {
            std::lock_guard<std::mutex> lg(coutMutex);
            std::cout << " [" << now() << "] Train " << id << " ARRIVED at " << seg.to << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); {
            std::lock_guard<std::mutex> lg(coutMutex);
            std::cout << " [" << now() << "] Train " << id << " LEFT " << seg.to << std::endl;
        }
        stLock.unlock();
    } {
        std::lock_guard<std::mutex> lg(coutMutex);
        std::cout << "[" << now() << "] Train " << id << " FINISHEd route " << std::endl;
    }
}


int main() {
    std::vector<Line> base1 = {
        {"Dushanbe", "Vahdat", 2},
        {"Vahdat", "Yovon", 3},
        {"Yovon", "Bokhtar", 3},
        {"Bokhtar", "Kulob", 4}
    };
    std::vector<Line> base2 = {
        {"Dushanbe", "Vahdat", 2},
        {"Vahdat", "Yovon", 3},
        {"Yovon", "Bokhtar", 3},
        {"Bokhtar", "Shaartuz", 4}
    };
    std::vector<Line> base3 = {
        {"Dushanbe", "Vahdat", 2},
        {"Vahdat", "Yovon", 3},
        {"Yovon", "Bokhtar", 3},
        {"Bokhtar", "Kolkhozobod", 3}
    };
    std::vector<std::vector<Line> > baseRoutes = {base1, base2, base3};
    for (auto &r: baseRoutes) {
        for (auto &l: r) {
            stationLocks.try_emplace(l.from);
            stationLocks.try_emplace(l.to);
        }
    }
    std::mt19937_64 rng(time(nullptr));
    std::vector<std::thread>trains;
    int train_count = 9;
    for (int i = 0; i < train_count; i++) {
        std::vector<Line> route = baseRoutes[i%baseRoutes.size()];
        if(!route.empty()) {
            std::uniform_int_distribution<int> dist(0, (int) route.size() - 1);
            int shift = dist(rng);
            rotate(route.begin(), route.begin() + shift, route.end());
        }
        if(i%2==1) {//каждый второй едет в обратную сторону
            std::vector<Line> rev;
            for(int j = route.size() - 1; j >= 0; j--) {
                rev.emplace_back(route[j].to, route[j].from, route[j].travelTime);
            }
            route.swap(rev);
        }
        if(!route.empty()) {
            int safety =0 ;
            while(route.front().from == route.back().to) {
                std::rotate(route.begin(), route.begin() + 1, route.end());
                ++safety;
            }
        }
        trains.emplace_back(trainThread, i+1, route);
    }
    for(auto &t: trains) t.join();{
        std::lock_guard<std::mutex> lg(coutMutex);
        std::cout<<"\n -----> Simulation OFF,the end ^_^!   <----- ";
    }
    return 0;
}

