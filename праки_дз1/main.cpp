//дз1: Локальный поиск для TSP и Рюкзака
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

//путь к папке с data и data2
string folder = "/Users/nickitazimin/Desktop/Праки/праки_дз1/праки_дз1/";



//РЮКЗАК

class Knapsack{
public:
    int n; //количество предметов
    int W; //вместимость рюкзака
    vector<int> v; //ценности
    vector<int> w; //веса

    Knapsack(string filename){
        ifstream file(filename);
        file >> n >> W;
        v.resize(n);
        w.resize(n);
        for (int i = 0; i < n; i++) {file >> v[i] >> w[i];}
    }

    //take[i] = 1 если предмет взяли, 0 если нет
    int value(const vector<int>& take) const{
        int sum = 0;
        for (int i = 0; i < n; i++) {if (take[i] == 1) sum += v[i];}
        return sum;
    }
    int weight(const vector<int>& take) const{
        int sum = 0;
        for (int i = 0; i < n; i++) {if (take[i] == 1) sum += w[i];}
        return sum;
    }
};


//базовый класс для всех методов решения рюкзака
class KnapsackSolver{
public:
    virtual vector<int> solve(const Knapsack& task) = 0;
    virtual ~KnapsackSolver() = default;
};


//локальный поиск + надстройка "случайное начальное приближение + множественные запуски"
class KnapsackLocalSearch: public KnapsackSolver{
    double time_limit; //сколько секунд даем на перезапуски
public:
    KnapsackLocalSearch(double T = 1): time_limit(T) {}

    //жадное начальное решение: каждый раз кладем предмет с самой большой ценностью на единицу веса, из тех, что еще влезают
    vector<int> greedy_start(const Knapsack& task){
        vector<int> take(task.n, 0);
        int weight = 0;
        while (true){
            int best = -1;
            for (int j = 0; j < task.n; j++){
                if (take[j] == 1 || weight + task.w[j] > task.W) continue;
                if (best == -1 || (double)task.v[j] / task.w[j] > (double)task.v[best] / task.w[best]) {best = j;}
            }
            if (best == -1) break; //больше ничего не влезает
            take[best] = 1;
            weight += task.w[best];
        }
        return take;
    }

    //случайное начальное решение: перемешиваем предметы и кладем по очереди все, что влезает
    vector<int> random_start(const Knapsack& task){
        vector<int> order(task.n);
        for (int i = 0; i < task.n; i++) {order[i] = i;}
        for (int i = task.n - 1; i > 0; i--){ //перемешивание
            int j = rand() % (i + 1);
            swap(order[i], order[j]);
        }
        vector<int> take(task.n, 0);
        int weight = 0;
        for (int k = 0; k < task.n; k++){
            int i = order[k];
            if (weight + task.w[i] <= task.W) {take[i] = 1; weight += task.w[i];}
        }
        return take;
    }

    //сам локальный поиск: пока можем улучшить решение маленьким изменением - улучшаем
    void improve(const Knapsack& task, vector<int>& take){
        int weight = task.weight(take);
        bool improved = true;
        while (improved){
            improved = false;

            //добавить предмет, если он влезает
            for (int j = 0; j < task.n; j++){
                if (take[j] == 0 && weight + task.w[j] <= task.W){
                    take[j] = 1;
                    weight += task.w[j];
                    improved = true;
                }
            }

            //обмен - вынуть предмет i и положить j, если j ценнее и влезает
            for (int i = 0; i < task.n && !improved; i++){
                if (take[i] == 0) continue;
                for (int j = 0; j < task.n; j++){
                    if (take[j] == 1) continue;
                    if (task.v[j] > task.v[i] && weight - task.w[i] + task.w[j] <= task.W){
                        take[i] = 0;
                        take[j] = 1;
                        weight = weight - task.w[i] + task.w[j];
                        improved = true;
                        break;
                    }
                }
            }
        }
    }

    vector<int> solve(const Knapsack& task) override{
        clock_t start = clock();
        //первый запуск - с жадного решения
        vector<int> best = greedy_start(task);
        improve(task, best);
        //дальше, пока есть время, - со случайных решений, запоминаем лучшее
        while ((double)(clock() - start) / CLOCKS_PER_SEC < time_limit){
            vector<int> take = random_start(task);
            improve(task, take);
            if (task.value(take) > task.value(best)) {best = take;}
        }
        return best;
    }
};



//TSP

class TSP{
public:
    int n; //количество городов
    vector<double> x, y; //координаты

    TSP(string filename){
        ifstream file(filename);
        file >> n;
        x.resize(n);
        y.resize(n);
        for (int i = 0; i < n; i++) {file >> x[i] >> y[i];}
    }

    double dist(int a, int b) const {return sqrt((x[a] - x[b]) * (x[a] - x[b]) + (y[a] - y[b]) * (y[a] - y[b]));}

    //длина замкнутого маршрута (route - порядок обхода городов)
    double length(const vector<int>& route) const{
        double sum = 0;
        for (int i = 0; i < n; i++) {sum += dist(route[i], route[(i + 1) % n]);}
        return sum;
    }
};


//базовый класс для методов решения TSP
class TSPSolver{
public:
    virtual vector<int> solve(const TSP& task) = 0;
    virtual ~TSPSolver() = default;
};


//локальный поиск 2-opt: убираем два ребра маршрута и переворачиваем кусок между ними
class TSPLocalSearch: public TSPSolver{
    double time_limit; //сколько секунд даем на улучшение маршрута
public:
    TSPLocalSearch(double T = 1): time_limit(T) {}

    //начальный маршрут: каждый раз идем в ближайший еще не посещенный город
    vector<int> nearest_start(const TSP& task){
        vector<int> route;
        vector<int> visited(task.n, 0);
        int cur = 0;
        route.push_back(cur);
        visited[cur] = 1;
        for (int step = 1; step < task.n; step++){
            int next = -1;
            for (int j = 0; j < task.n; j++){
                if (visited[j] == 0 && (next == -1 || task.dist(cur, j) < task.dist(cur, next))) {next = j;}
            }
            route.push_back(next);
            visited[next] = 1;
            cur = next;
        }
        return route;
    }

    //перевернуть кусок маршрута с i по j
    void reverse_part(vector<int>& route, int i, int j){
        while (i < j){
            swap(route[i], route[j]);
            i++;
            j--;
        }
    }

    void two_opt(const TSP& task, vector<int>& route){
        int n = task.n;
        clock_t start = clock();
        bool improved = true;
        while (improved){
            improved = false;
            for (int i = 0; i < n - 1; i++){
                for (int j = i + 2; j < n; j++){
                    //ребра a,b и c,d заменяем на a,c и b,d
                    int a = route[i], b = route[i + 1];
                    int c = route[j], d = route[(j + 1) % n];
                    if (a == d) continue; //эти ребра соседние, менять нечего
                    double delta = task.dist(a, c) + task.dist(b, d) - task.dist(a, b) - task.dist(c, d);
                    if (delta < -1e-9){
                        reverse_part(route, i + 1, j);
                        improved = true;
                    }
                }
                if ((double)(clock() - start) / CLOCKS_PER_SEC > time_limit) return; //время вышло
            }
        }
    }

    vector<int> solve(const TSP& task) override{
        vector<int> route = nearest_start(task);
        two_opt(task, route);
        return route;
    }
};



int main(){
    srand(1);

    cout << "Рюкзак, локальный поиск" << endl;
    string ks_files[] = {"ks_4_0", "ks_19_0", "ks_30_0", "ks_40_0", "ks_45_0", "ks_50_0", "ks_50_1", "ks_60_0", "ks_100_0", "ks_100_1", "ks_100_2", "ks_200_0", "ks_200_1", "ks_300_0", "ks_400_0", "ks_500_0", "ks_1000_0", "ks_10000_0"};
    KnapsackLocalSearch ks_solver(1); //по 1 секунде на задачу
    for (string name : ks_files){
        Knapsack task(folder + "data/" + name);
        clock_t start = clock();
        vector<int> take = ks_solver.solve(task);
        double time = (double)(clock() - start) / CLOCKS_PER_SEC;
        cout << name << ": ценность = " << task.value(take) << ", вес = " << task.weight(take)
             << " (W = " << task.W << "), время = " << time << " c" << endl;
    }

    cout << endl << "TSP, локальный поиск 2-opt" << endl;
    //самые большие считаются несколько минут
    string tsp_files[] = {"tsp_5_1", "tsp_51_1", "tsp_70_1", "tsp_76_1", "tsp_76_2", "tsp_99_1", "tsp_100_1", "tsp_100_2", "tsp_100_3", "tsp_100_4", "tsp_100_5", "tsp_100_6", "tsp_101_1", "tsp_105_1", "tsp_107_1", "tsp_124_1","tsp_127_1", "tsp_136_1", "tsp_144_1", "tsp_150_1", "tsp_150_2", "tsp_152_1", "tsp_159_1", "tsp_195_1", "tsp_198_1", "tsp_200_1", "tsp_200_2", "tsp_225_1", "tsp_226_1", "tsp_262_1", "tsp_264_1", "tsp_299_1", "tsp_318_1", "tsp_318_2", "tsp_400_1", "tsp_417_1", "tsp_439_1", "tsp_442_1", "tsp_493_1", "tsp_574_1", "tsp_575_1", "tsp_654_1", "tsp_657_1", "tsp_724_1", "tsp_783_1", "tsp_1000_1", "tsp_1060_1", "tsp_1084_1", "tsp_1173_1", "tsp_1291_1", "tsp_1304_1", "tsp_1323_1", "tsp_1379_1", "tsp_1400_1", "tsp_1432_1", "tsp_1577_1", "tsp_1655_1", "tsp_1748_1", "tsp_1817_1", "tsp_1889_1", "tsp_2103_1", "tsp_2152_1", "tsp_2319_1", "tsp_2392_1", "tsp_3038_1", "tsp_3795_1", "tsp_4461_1", "tsp_5915_1", "tsp_5934_1", "tsp_7397_1", "tsp_9432_1", "tsp_11849_1", "tsp_14051_1", "tsp_18512_1", "tsp_33810_1", "tsp_85900_1"};
    TSPLocalSearch tsp_solver(1); //по 1 секунде на улучшение маршрута
    for (string name : tsp_files){
        TSP task(folder + "data2/" + name);
        clock_t start = clock();
        vector<int> route = tsp_solver.solve(task);
        double time = (double)(clock() - start) / CLOCKS_PER_SEC;
        cout << name << ": длина маршрута = " << task.length(route) << ", время = " << time << " c" << endl;
    }

    return 0;
}
