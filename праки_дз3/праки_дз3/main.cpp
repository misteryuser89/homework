#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iomanip> //setw - чтобы столбцы таблицы были ровными
using namespace std;

//путь к папке с data
string folder = "/Users/nickitazimin/Desktop/Праки/праки_дз3/праки_дз3/";



class Knapsack{
public:
    int n;
    int W;
    vector<int> v;
    vector<int> w;

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


//базовый класс, solve возвращает максимальную ценность
class KnapsackSolver{
public:
    virtual int solve(const Knapsack& task) = 0;
    virtual ~KnapsackSolver() = default;
};



//ЖАДНЫЙ АЛГОРИТМ (дз3)

//как в лекции: пока есть предметы, которые влезают, берем из них самый "лучший"
//что значит "лучший" - решают наследники через priority
class Greedy: public KnapsackSolver{
public:
    virtual double priority(const Knapsack& task, int i) = 0; //чем больше, тем лучше предмет

    int solve(const Knapsack& task) override{
        vector<int> take(task.n, 0);
        int weight = 0;
        int best = 0; //любое число, кроме -1, чтобы зайти в цикл
        while (best != -1){
            best = -1; //на каждом шаге ищем лучший предмет заново
            for (int j = 0; j < task.n; j++){
                if (take[j] == 1 || weight + task.w[j] > task.W) continue;
                if (best == -1 || priority(task, j) > priority(task, best)) best = j;
            }
            if (best != -1) {
                take[best] = 1;
                weight += task.w[best];
            }
        }
        return task.value(take);
    }
};

//жадность по ценности: берем самый ценный предмет
class GreedyByValue: public Greedy{
public:
    double priority(const Knapsack& task, int i) override {return task.v[i];}
};

//жадность по калорийности: берем предмет с самой большой ценностью на единицу веса
class GreedyByRatio: public Greedy{
public:
    double priority(const Knapsack& task, int i) override {return (double)task.v[i] / task.w[i];}
};



//ЛОКАЛЬНЫЙ ПОИСК (дз1)

class LocalSearch: public KnapsackSolver{
    double time_limit; //сколько секунд даем на перезапуски
public:
    LocalSearch(double T = 1): time_limit(T) {}

    //жадное начальное решение по калорийности
    vector<int> greedy_start(const Knapsack& task){
        vector<int> take(task.n, 0);
        int weight = 0;
        while (true){
            int best = -1;
            for (int j = 0; j < task.n; j++){
                if (take[j] == 1 || weight + task.w[j] > task.W) continue;
                if (best == -1 || (double)task.v[j] / task.w[j] > (double)task.v[best] / task.w[best]) {best = j;}
            }
            if (best == -1) break;
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

    //пока можем улучшить решение маленьким изменением - улучшаем
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

    int solve(const Knapsack& task) override{
        clock_t start = clock();
        vector<int> best = greedy_start(task);
        improve(task, best);
        while ((double)(clock() - start) / CLOCKS_PER_SEC < time_limit){
            vector<int> take = random_start(task);
            improve(task, take);
            if (task.value(take) > task.value(best)) {best = take;}
        }
        return task.value(best);
    }
};



//метод ветвей и границ

class BranchAndBound: public KnapsackSolver{
    int n, W;
    vector<int> v, w; //предметы, отсортированные по убыванию v/w
    int best; //лучшая найденная ценность
public:
    
    double bound(int k, int value, int weight){
        double result = value;
        int free = W - weight;
        for (int i = k; i < n; i++){
            if (w[i] <= free) {free -= w[i]; result += v[i];}
            else {result += (double)v[i] * free / w[i]; break;}
        }
        return result;
    }

    //k - номер предмета, про который решаем: брать или нет
    void branch(int k, int value, int weight){
        if (value > best) best = value;
        if (k == n) return;
        if (bound(k, value, weight) < best + 1) return; //ветка не даст больше best - отсекаем
        if (weight + w[k] <= W) branch(k + 1, value + v[k], weight + w[k]); //берем
        branch(k + 1, value, weight); //не берем
    }

    int solve(const Knapsack& task) override{
        n = task.n; W = task.W; v = task.v; w = task.w;
        //сортировка по убыванию v/w
        for (int i = 0; i < n; i++){
            for (int j = i + 1; j < n; j++){
                if ((double)v[j] / w[j] > (double)v[i] / w[i]) {swap(v[i], v[j]); swap(w[i], w[j]);}
            }
        }
        best = 0;
        branch(0, 0, 0);
        return best;
    }
};



//генетический алгоритм (дз2)

class Genetic: public KnapsackSolver{
    int population_size; //особей в поколении
    int generations; //поколений
public:
    Genetic(int P = 100, int G = 100): population_size(P), generations(G) {}

    //приспособленность особи = ценность рюкзака, если перегружен - 0
    int fitness(const Knapsack& task, const vector<int>& genes){
        int value = 0, weight = 0;
        for (int i = 0; i < task.n; i++){
            if (genes[i] == 1) {value += task.v[i]; weight += task.w[i];}
        }
        if (weight > task.W) return 0;
        return value;
    }

    //случайная особь: кладем случайные предметы, пока влезают
    vector<int> random_genes(const Knapsack& task){
        vector<int> genes(task.n, 0);
        int weight = 0;
        for (int t = 0; t < task.n; t++){
            int i = rand() % task.n;
            if (genes[i] == 0 && weight + task.w[i] <= task.W) {genes[i] = 1; weight += task.w[i];}
        }
        return genes;
    }

    int solve(const Knapsack& task) override{
        vector<vector<int>> population;
        vector<int> fit;
        for (int i = 0; i < population_size; i++){
            population.push_back(random_genes(task));
            fit.push_back(fitness(task, population[i]));
        }

        for (int g = 0; g < generations; g++){
            //лучшее переходит в новое поколение без изменений
            int best = 0;
            for (int i = 1; i < population_size; i++) {if (fit[i] > fit[best]) best = i;}
            vector<vector<int>> new_population;
            vector<int> new_fit;
            new_population.push_back(population[best]);
            new_fit.push_back(fit[best]);

            while ((int)new_population.size() < population_size){
                //из двух случайных берем лучшего
                int a = rand() % population_size, b = rand() % population_size;
                int p1 = (fit[a] > fit[b]) ? a : b;
                a = rand() % population_size; b = rand() % population_size;
                int p2 = (fit[a] > fit[b]) ? a : b;

                //начало от первого родителя, конец от второго
                int cut = rand() % task.n;
                vector<int> child(task.n);
                for (int i = 0; i < task.n; i++) {child[i] = (i < cut) ? population[p1][i] : population[p2][i];}

                //меняем один случайный ген
                int m = rand() % task.n;
                child[m] = 1 - child[m];

                new_population.push_back(child);
                new_fit.push_back(fitness(task, child));
            }
            population = new_population;
            fit = new_fit;
        }

        int best = 0;
        for (int i = 1; i < population_size; i++) {if (fit[i] > fit[best]) best = i;}
        return fit[best];
    }
};



int main(){
    srand(1);
    const int F = 18; //количество файлов
    const int M = 5; //количество методов
    string files[F] = {"ks_4_0", "ks_19_0", "ks_30_0", "ks_40_0", "ks_45_0", "ks_50_0", "ks_50_1", "ks_60_0", "ks_100_0", "ks_100_1", "ks_100_2", "ks_200_0", "ks_200_1", "ks_300_0", "ks_400_0", "ks_500_0", "ks_1000_0", "ks_10000_0"};

    KnapsackSolver* methods[M] = {new GreedyByValue(), new GreedyByRatio(), new LocalSearch(1), new BranchAndBound(), new Genetic(100, 100)};

    int value[F][M];
    double time[F][M];

    for (int f = 0; f < F; f++){
        Knapsack task(folder + "data/" + files[f]);
        for (int m = 0; m < M; m++){
            clock_t start = clock();
            value[f][m] = methods[m]->solve(task);
            time[f][m] = (double)(clock() - start) / CLOCKS_PER_SEC;
        }
    }

    //таблица 1: набранная ценность
    cout << "Ценность" << endl;
    cout << "файл            жадн.ценн.   жадн.калор.     лок.поиск    ветв.и гр.      генетич." << endl;
    for (int f = 0; f < F; f++){
        cout << left << setw(12) << files[f] << right;
        for (int m = 0; m < M; m++) {cout << setw(14) << value[f][m];}
        cout << endl;
    }

    //таблица 2: время, 6 знаков после запятой
    cout << endl << "Время, с" << endl;
    cout << "файл            жадн.ценн.   жадн.калор.     лок.поиск    ветв.и гр.      генетич." << endl;
    cout << fixed;
    cout.precision(6);
    for (int f = 0; f < F; f++){
        cout << left << setw(12) << files[f] << right;
        for (int m = 0; m < M; m++) {cout << setw(14) << time[f][m];}
        cout << endl;
    }

    for (int m = 0; m < M; m++) {delete methods[m];}
    return 0;
}
