//Метод ветвей и границ для рюкзака, Генетический алгоритм для рюкзака
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;

//путь к папке с data
string folder = "/Users/nickitazimin/Desktop/Праки/праки_дз2/праки_дз2/";



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
};


//базовый класс, solve возвращает максимальную ценность
class KnapsackSolver{
public:
    virtual int solve(const Knapsack& task) = 0;
    virtual ~KnapsackSolver() = default;
};



//МЕТОД ВЕТВЕЙ И ГРАНИЦ

class BranchAndBound: public KnapsackSolver{
    int n, W;
    vector<int> v, w; //предметы, отсортированные по убыванию v/w
    int best; //лучшая найденная ценность
public:
    //оценка сверху: докладываем предметы по порядку, последний который не влезает - кусочком
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



//ГЕНЕТИЧЕСКИЙ АЛГОРИТМ

class Genetic: public KnapsackSolver{
    int population_size; //сколько особей в поколении
    int generations; //сколько поколений
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
    string files[] = {"ks_4_0", "ks_19_0", "ks_30_0", "ks_40_0", "ks_45_0", "ks_50_0", "ks_50_1", "ks_60_0", "ks_100_0", "ks_100_1", "ks_100_2", "ks_200_0", "ks_200_1", "ks_300_0", "ks_400_0", "ks_500_0", "ks_1000_0", "ks_10000_0"};

    BranchAndBound bnb;
    Genetic genetic(100, 100);

    for (string name : files){
        Knapsack task(folder + "data/" + name);

        clock_t start = clock();
        int value1 = bnb.solve(task);
        double time1 = (double)(clock() - start) / CLOCKS_PER_SEC;

        start = clock();
        int value2 = genetic.solve(task);
        double time2 = (double)(clock() - start) / CLOCKS_PER_SEC;

        cout << name << ": ветви и границы = " << value1 << " (" << time1 << " c), "
             << "генетический = " << value2 << " (" << time2 << " c)" << endl;
    }
    return 0;
}
