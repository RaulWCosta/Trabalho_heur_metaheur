#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <dirent.h>
#include <vector>

using namespace std;

unsigned char isFile =0x8;

int INT_BIG = 20000000;

class Instance {
    string package;
    int num_locais, num_clientes;
    vector<vector<int>> matrix;
    vector<int> opening_cost_vector;

    public:
        Instance(string file_path, string pkg) {
            /*
            Inicializa objeto contendo informações de uma instancia.
            */
            package = pkg;
            if (package == "BildeKrarup") {
                init_bilde_krarup_instance(file_path);
            }
        }

	vector<int> run_tabu_search(vector<int> initial_solution, int tabu_size, int counter_limit) {
            /*
		Executa o metodo de tabu search para melhorar o resultado obtido pela heuristica construtiva.
		Parametros:
			tabu_size: numero de itens na lista tabu
			counter_limit: numero maximo de iteracoes do algoritmo sem ocorrer nenhuma melhoria no melhor custo
            */
            vector<int> current_solution = initial_solution;
            int best_cost = calc_total_cost(current_solution);
            vector<int> tabu_list(num_locais, 0);
            int counter = 0;
            int curr_cost, flip_idx, best_flip_idx;
            srand(42); //set seed
            while (counter < counter_limit) {
                // for (int item: current_solution) {
                //     cout << item << ", ";
                // }
                // cout << endl;
                // cout << "counter = " << counter << ", best_cost = " << best_cost << endl;
                best_flip_idx = get_best_flip(current_solution, tabu_list);
                current_solution[best_flip_idx] = 1 - current_solution[best_flip_idx];
                curr_cost = calc_total_cost(current_solution);
                // cout << "update_tabu" << endl;
                // for (int item: tabu_list) {
                //     cout << item << ", ";
                // }
                // cout << endl;
                update_tabu_search(tabu_list);
                tabu_list[best_flip_idx] = tabu_size;
                if (curr_cost < best_cost) {
                    best_cost = curr_cost;
                    counter = 0;
                } else {
                    // desfaz o best flip, pois a solucao nao eh melhor do que a solucao atual
                    current_solution[best_flip_idx] = 1 - current_solution[best_flip_idx];
                    // escolhe um local randomico que esta aberto e fecha
                    flip_idx = rand() % num_locais;
                    if (get_num_instalacoes(current_solution) > 0) {
                        while (!current_solution[flip_idx])
                            flip_idx = rand() % num_locais;
                        current_solution[flip_idx] = 0;
                    }
                    counter++;
                }
            }
            cout << "\ttabu search = " << best_cost << endl;
            cout << "\ttabu search = " << calc_total_cost(current_solution) << endl;

            return current_solution;
        }

        vector<int> solve_constructive_heuristic(int rand_seed, float alpha) {
            /*
            Executa um algoritmo guloso para obter um conjunto de intalacoes.
            O algoritmo vai somar, para cada local, o custo de abertura e os custos de conexao
                para um dado cliente.
            Em seguida esse vetor será ordenado e cada valor será iterado. Se o custo total
                for reduzido com a abertura de uma nova intalacao, a intalacao é adicionada
                ao vetor que será retornado.
            Esse processo é refeito para cada cliente de maneira iterativa.
            Parametros:
                rand_seed: semente para gerar valores aleatorios.
                alpha: parametro que determina a intensificacao vs diversificacao do algoritmo.
                    alpha = 0 -> puramente gulosa
                    alpha -> puramente aleatoria
            */
            vector<int> location_total_cost(num_locais, 0);
            vector<int> selected_locations(num_locais, 0);
            vector<int> sorted_idxs;
            int new_cost = 0;
            int curr_cost = INT_BIG;
            int candidate_idx;
            int k;
            srand(rand_seed);
            // qtd de locais candidatos para atender um dado cliente
            int num_candidate_loc = (int) (num_locais * alpha);
            for (int j = 0; j < num_clientes; j++) {
                // calcula quais sao os 'best_loc_num' locais que serão considerados
                for (int i = 0; i < num_locais; i++) {
                    location_total_cost[i] = opening_cost_vector[i] + matrix[i][j];
                }
                k = (rand() % num_candidate_loc) + 1;
                candidate_idx = get_k_min_idx(k, location_total_cost, selected_locations);
                selected_locations[candidate_idx] = 1;
                new_cost = calc_total_cost(selected_locations);
                if (new_cost < curr_cost) {
                    curr_cost = new_cost;
                } else {
                    selected_locations[candidate_idx] = 0;
                }
            }
            //cout << "\tconstructive = " << curr_cost << endl;
            return selected_locations;
        }

        /**
         * Retorna as melhores QTY_SOLUCTIONS considerando o número de tentativas "k"
         * e o parâmetro de intensificação/diversificação "alpha"
         **/
        vector <vector<int>> solve_grasp(float alpha, int k, int QTY_SOLUTIONS) {

            vector <vector<int>> best_solutions(QTY_SOLUTIONS, vector<int>(num_locais, 0));
            vector<int> selected_locations;

            for(int i=0; i<k; i++){
                selected_locations = solve_constructive_heuristic(i, alpha);

                //pega o custo da "n-ésima" (QTY_SOLUTIONS) melhor solução até então
                int cost_n_best_solution = calc_total_cost(best_solutions[QTY_SOLUTIONS-1]);
                int cost_selected_solution = calc_total_cost(selected_locations);
                if ( ( cost_selected_solution < cost_n_best_solution)
                        || (cost_n_best_solution == 0) ){
                        //posiciona a solução encontrada no vetors de best_solutions
                        // na ordem crescente de custo
                        int pos = QTY_SOLUTIONS-1;
                        for(int aux = 0; aux<(QTY_SOLUTIONS-1) ; aux++){}
                            if ( cost_selected_solution < calc_total_cost(best_solutions[aux]) ) {
                                pos = aux;
                                break;
                            }     
                        }
                        for(int aux=QTY_SOLUTIONS-1; aux>pos; aux--)
                            best_solutions[aux] = best_solutions[aux-1];
                        
                        best_solutions[pos] = selected_locations;
                }
            }
            return best_solutions;
        }



        int calc_total_cost(vector<int> selected_locations) {
            /*
            Calcula custo total da funcao que o algoritmo quer minimizar.
            O custo total é composto pela soma dos custos de abertura das instalacoes abertas mais o menor custo de
            conexao entre as instalacoes abertas e cada cliente.
            */
            if (get_num_instalacoes(selected_locations) == 0) {
                return INT_BIG;
            }
            vector<int> current_min_vector(num_clientes, INT_BIG);
            int total_cost = 0;
            for (int i = 0; i < num_locais; i++) {
                if (selected_locations[i]) {
                    for (int j = 0; j < num_clientes; j++) {
                        if (matrix[i][j] < current_min_vector[j]) {
                            current_min_vector[j] = matrix[i][j];
                        }
                    }
                }
            }

            for (int i = 0; i < num_clientes; i++)
                total_cost += current_min_vector[i];

            for (int i = 0; i < num_locais; i++)
                total_cost += selected_locations[i] * opening_cost_vector[i];

            return total_cost;
        }
    private:

	void update_tabu_search(vector<int> &tabu_list) {
            /**/
            for (int i = 0; i < num_locais; i++) {
                if (tabu_list[i] > 0)
                    tabu_list[i]--;
            }
        }

        int get_best_flip(vector<int> current_solution, vector<int> tabu_list) {
            /**/
            int min_val = INT_BIG;
            int min_idx;
            int curr_val;
            for (int i = 0; i < num_locais; i++) {
                if (tabu_list[i] == 0) {
                    // flip valor da pos i da current solution;
                    current_solution[i] = 1 - current_solution[i];
                    curr_val = calc_total_cost(current_solution);
                    if (curr_val < min_val) {
                        min_val = curr_val;
                        min_idx = i;
                    }
                    current_solution[i] = 1 - current_solution[i];
                }
            }
            return min_idx;
        }

        vector<string> split(string s, string delimiter) {
            /*
            Quebra uma string 's' em substrings baseado em um separador passado.
            */
            size_t pos_start = 0, pos_end, delim_len = delimiter.length();
            string token;
            vector<string> res;

            while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
                token = s.substr (pos_start, pos_end - pos_start);
                pos_start = pos_end + delim_len;
                if (!token.empty())
                    res.push_back (token);
            }
            if (!s.substr(pos_start).empty())
                res.push_back(s.substr(pos_start));
            return res;
        }

        void init_bilde_krarup_instance(string file_path) {
            /*
            Le as informaçes de uma instancia do conjunto BildeKrarup recebidos em um arquivo.
            */
            fstream file_obj;
            vector<string> words_in_line;
            file_obj.open(file_path, ios::in);
            string line = "";
            getline(file_obj, line);
            getline(file_obj, line);
            words_in_line = split(line, " ");
            num_locais = stoi(words_in_line.at(0));
            num_clientes = stoi(words_in_line.at(1));
            // cout << num_locais << ", " << num_clientes << endl;

            matrix.resize(num_locais);
            for (int i = 0; i < num_locais; i++) {
                matrix[i].resize(num_clientes);
            }
            opening_cost_vector.resize(num_locais);
            int aux_val;
            int cont = 0;

            while(getline(file_obj, line)) {
                words_in_line = split(line, " ");
                opening_cost_vector[cont] = stoi(words_in_line[1]);

                for (int i = 2; i < words_in_line.size(); i++) {
                    matrix[cont][i-2] = stoi(words_in_line.at(i));
                }
                cont++;
            }
            // cout << "matrix obtida." << endl;
            // for (int i = 0; i < matrix.size(); i++) {
            //     for (int j = 0; j < matrix[i].size(); j++) {
            //         cout << matrix[i][j] << " ";
            //     }
            //     cout << endl;
            // }
        }

        int get_num_instalacoes(vector<int> selected_locations) {
            int loc_counter = 0;
            for (int loc: selected_locations) {
                loc_counter += loc;
            }
            return loc_counter;
        }


        int get_k_min_idx(int k, vector<int> location_total_cost, vector<int> sel_locs) {
            /*
            Retorna o indice do k-esimo valor do vetor 'location_total_cost'.
                Os valores 'sel_locs' == 1 são ignorados nessa contagem.
            */
            int min_val = INT_BIG;
            int min_idx = -1;
            while (k--) {
                min_val = INT_BIG;
                min_idx = -1;
                for (int j = 0; j < num_locais; j++) {
                    if (sel_locs[j] == 0 && location_total_cost[j] < min_val) {
                        min_val = location_total_cost[j];
                        min_idx = j;
                    }
                }
                sel_locs[min_idx] = 1;
            }
            return min_idx;
        }

};


void findDataFiles(string folder, vector <string> *files){

    DIR *dir;
	struct dirent *diread;

	if ( (dir = opendir(folder.c_str())) != nullptr){

		while( (diread = readdir(dir)) != nullptr) {
			if ( strcmp(diread->d_name, "files.lst") == 0 ){
				//abre o arquivo "files.lst" e pega a lista de arquivos com os problemas para analisar
				string file_path;
				string line = "";
				file_path = folder + "/files.lst";
		        fstream file_obj;
				file_obj.open(file_path, ios::in);
		        while(getline(file_obj, line)) {
					string full_path;
				    full_path = folder + "/" + line;
					files->push_back(full_path);
				}

			}else{
				if ( (diread->d_type != isFile)
						&&  (strcmp(diread->d_name,".")  != 0)
						&&  (strcmp(diread->d_name, "..") != 0) ){
					string aux_folder = folder +"/"+ string(diread->d_name);
					findDataFiles( aux_folder, files);
				}
			}
		}

	}

	return;
}


int main(void) {
    string folderRoot = "./data/BildeKrarup/B";
    vector<string> files;

    findDataFiles(folderRoot, &files);


    Instance *inst;
    string file_name = files.at(0);//"./data/BildeKrarup/B/B1.1";


    // string path = "./data/BildeKrarup/";
    // for (const auto & entry : fs::directory_iterator(path))
    //     std::cout << entry.path() << std::endl;
    // }

    //string pkg = "BildeKrarup";
    //inst = new Instance(file_name, pkg);
    vector<int> locations;
    vector<vector<int>> best_solutions;
    //for (int loc: locations)
    //    cout << loc << ", ";
    //cout << endl;

    for (string file: files) {
        cout << file << endl;
        inst = new Instance(file, "BildeKrarup");
        locations = inst->solve_constructive_heuristic(42, 0.5);
        cout << "\tconstructive = " << inst->calc_total_cost(locations) << endl;

        locations = inst->run_tabu_search(locations, 10, 500);
        cout << "\ttabu search = " << inst->calc_total_cost(locations) << endl;
        
        best_solutions = inst->solve_grasp(0.4, 60, 5);
        cout << "\tgrasp = " << inst->calc_total_cost(best_solutions[0]) << endl;

    }
    cout << files.size() << endl;
    return 0;
}
