#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <vector>
#include <dirent.h>

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
            cout << "tabu search, " << best_cost << endl;
            return current_solution;
        }
	
        vector<int> solve_constructive_heuristic() {
            /*
            Executa um algoritmo guloso para obter um conjunto de intalacoes.
            O algoritmo vai somar, para cada local, o custo de abertura e os custos de conexao
                com cada cliente.
            Em seguida esse vetor será ordenado e cada valor será iterado. Se o custo total
                for reduzido com a abertura de uma nova intalacao, a intalacao é adicionada
                ao vetor que será retornado.
            */
            vector<int> location_total_cost(num_locais);
            vector<int> selected_locations(num_locais, 0);
            for (int i = 0; i < num_locais; i++) {
                location_total_cost[i] = opening_cost_vector[i];
                for (int val: matrix[i])
                    location_total_cost[i] += val;
            }
            int new_cost = 0;
            int curr_cost = INT_BIG;
            // cout << "todos custos totais = ";
            // for (int val: location_total_cost) {
            //     cout << val << ", ";
            // }
            // cout << endl;

            vector<int> sorted_idxs = get_sorted_idxs(location_total_cost, selected_locations);
            for (int min_idx: sorted_idxs) {
                selected_locations[min_idx] = 1;
                new_cost = calc_total_cost(selected_locations);
                if (new_cost < curr_cost) {
                    curr_cost = new_cost;
                } else {
                    selected_locations[min_idx] = 0;
                }
            }

            // cout << "custos totais selecionados = ";
            // for (int i = 0; i < num_locais; i++) {
            //     if (selected_locations[i])
            //         cout << "[" << i << ", " << location_total_cost[i] << "], ";
            // }
            // cout << endl;
            cout << "constructive, " << curr_cost << endl;
            return selected_locations;
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
            string line = "teste";
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

        vector<int> get_sorted_idxs(vector<int> location_total_cost, vector<int> selected_locations) {
            /*
            Retorna um vetor com as idxs do vetor 'location_total_cost' ordenadas de menor valor para maior valor.
            Os indices que estao contidos no vetor 'selected_locations' sao ignorados.
            */
            int min_val = INT_BIG;
            int min_idx = -1;
            int cont_num_locais = num_locais;
            vector<int> sorted_idxs;

            while (cont_num_locais--) {
                min_val = INT_BIG;
                min_idx = -1;
                for (int j = 0; j < num_locais; j++) {
                    if (!selected_locations[j] && location_total_cost[j] < min_val) {
                        min_val = location_total_cost[j];
                        min_idx = j;
                    }
                }
                sorted_idxs.push_back(min_idx);
                selected_locations[min_idx] = 1;
            }
            return sorted_idxs;
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
    string folderRoot = "./data/BildeKrarup";
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
    //for (int loc: locations)
    //    cout << loc << ", ";
    //cout << endl;
	
    for (string file: files) {
        cout << file << endl;
        inst = new Instance(file, "BildeKrarup");
        locations = inst->solve_constructive_heuristic();
        locations = inst->run_tabu_search(locations, 10, 500);
    }
    cout << files.size() << endl;
    return 0;
}
