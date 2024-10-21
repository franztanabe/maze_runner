#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>

// Definição de um tipo para representar o labirinto como uma matriz de caracteres
using Maze = std::vector<std::vector<char>>;

// Estrutura que define uma posição no labirinto
struct Position {
  int row;
  int col;
};

// Variáveis globais
Maze maze;                          // Labirinto representado como matriz
int maze_rows, maze_cols;            // Dimensões do labirinto
std::queue<Position> open_positions; // Fila de posições a serem exploradas
bool exit_found = false;             // Indica se a saída foi encontrada
std::mutex maze_mutex;               // Controle de concorrência

// Função para carregar o labirinto a partir de um arquivo
Position load_maze(const std::string &file_name) {
  std::ifstream file(file_name);
  if (!file.is_open()) {
    std::cerr << "Erro ao abrir o arquivo: " << file_name << std::endl;
    return {-1, -1}; // Retorna posição inválida em caso de erro
  }

  // Lê as dimensões do labirinto
  file >> maze_rows >> maze_cols;
  maze.resize(maze_rows, std::vector<char>(maze_cols)); // Redimensiona a matriz

  // Preenche o labirinto com os caracteres lidos do arquivo
  for (int i = 0; i < maze_rows; ++i) {
    for (int j = 0; j < maze_cols; ++j) {
      file >> maze[i][j];
    }
  }
  file.close();

  // Encontra a posição inicial ('e') no labirinto
  for (int i = 0; i < maze_rows; ++i) {
    for (int j = 0; j < maze_cols; ++j) {
      if (maze[i][j] == 'e') {
        return {i, j}; // Retorna a posição inicial
      }
    }
  }

  return {-1, -1}; // Retorna posição inválida se não encontrar 'e'
}

// Função para imprimir o estado atual do labirinto
void display_maze() {
  std::cout << "\033[2J\033[1;1H"; // Limpa a tela
  for (int i = 0; i < maze_rows; ++i) {
    for (int j = 0; j < maze_cols; ++j) {
      std::cout << maze[i][j];
    }
    std::cout << '\n'; // Quebra de linha ao final de cada linha do labirinto
  }
}

// Verifica se uma posição é válida para ser explorada
bool is_valid_position(int row, int col) {
  // Verifica se está dentro dos limites do labirinto e se é um caminho livre ('x' ou 's')
  return (row >= 0 && row < maze_rows && col >= 0 && col < maze_cols &&
          (maze[row][col] == 'x' || maze[row][col] == 's'));
}

// Função que realiza a exploração do labirinto
void explore_maze() {
  while (true) {
    Position current_pos;

    {
      std::lock_guard<std::mutex> lock(maze_mutex); // Bloqueia a fila para acesso exclusivo
      if (exit_found || open_positions.empty()) {
        return; // Termina se a saída foi encontrada ou não há mais posições a explorar
      }

      current_pos = open_positions.front();
      open_positions.pop();
    }

    // Verifica se a posição atual é a saída
    if (maze[current_pos.row][current_pos.col] == 's') {
      std::lock_guard<std::mutex> lock(maze_mutex);
      exit_found = true;
      return; // Termina quando a saída é encontrada
    }

    // Marca a posição atual como visitada
    maze[current_pos.row][current_pos.col] = '.';
    display_maze(); // Exibe o estado atualizado do labirinto
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Pequena pausa para visualização

    // Verifica as posições adjacentes (cima, baixo, esquerda, direita)
    Position neighbors[] = {
        {current_pos.row - 1, current_pos.col}, // Cima
        {current_pos.row + 1, current_pos.col}, // Baixo
        {current_pos.row, current_pos.col - 1}, // Esquerda
        {current_pos.row, current_pos.col + 1}  // Direita
    };

    {
      std::lock_guard<std::mutex> lock(maze_mutex);
      for (const auto &neighbor : neighbors) {
        if (is_valid_position(neighbor.row, neighbor.col)) {
          open_positions.push(neighbor); // Adiciona posição válida à fila
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Uso: " << argv[0] << " <arquivo_labirinto>" << std::endl;
    return 1;
  }

  Position start_position = load_maze(argv[1]); // Carrega o labirinto do arquivo
  if (start_position.row == -1 || start_position.col == -1) {
    std::cerr << "Posição inicial não encontrada no labirinto." << std::endl;
    return 1;
  }

  // Inicializa a fila com a posição inicial
  open_positions.push(start_position);

  // Cria e inicia múltiplas threads para explorar o labirinto
  std::vector<std::thread> threads(5);
  for (auto &thread : threads) {
    thread = std::thread(explore_maze);
  }

  // Aguarda todas as threads finalizarem
  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // Exibe o resultado final
  if (exit_found) {
    std::cout << "Saída encontrada!" << std::endl;
  } else {
    std::cout << "Não foi possível encontrar a saída." << std::endl;
  }

  return 0;
}
