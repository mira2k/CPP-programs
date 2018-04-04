/** Кривошапко Мария, 699 группа.
 * Даны регулярное выражение в обратной польской нотации 𝛼 и слово 𝑢 ∈ { 𝑎, 𝑏, 𝑐 }*
 * Найти длину самого длинного подслова слова 𝑢, являющегося также подсловом некоторого слова в
 * 𝐿(𝛼).
 * Cуть алгоритма: построить e-NFA автомат для регулярного выражения, данного в условии. Для этого автомата построить
 * автомат его подслов. Полученным автоматом будем обрабатывать слово с каждой позиции слова dfs-ом. Для того, чтобы
 * не пройти по циклу из эпсилон-ребер храним матрицу visited[text_position][state_index], показывающую пришли ли мы
 * в состояние state_index из позиции text_position
 */

#include <iostream>
#include <stack>
#include <string>
#include <vector>

using namespace std;

struct Edge {
    int v_;
    string letter_;
    Edge(int to, string letter): v_(to), letter_(letter) {}
};
class AutomatonBuilder {
private:
    size_t vertices_num_;
    vector<vector<Edge> > transitions_;
    vector<bool> terminals_;
    int FindFirstBad(int node_index,
                     int position,
                     const string& word,
                     vector< vector<bool> >& visited ) const {
        /** Возвращает первую позицию, не входящую в подслово,
         * начинающееся с позиции position и состояния node_index
         * длина max_length будет пересчитываться по формуле first_bad(результат программы) - position.
         */
        if( position == word.length() ) {
            return position;
        }
        visited[position][node_index] = true;
        int result = position;
        for (int j = 0; j < transitions_[node_index].size(); ++j) {
            string new_letter = transitions_[node_index][j].letter_;
            int new_v = transitions_[node_index][j].v_;
            int new_position = (new_letter == "") ? position : position + 1;
            // как видим, при переходе по эпсилон-ребру позиция текста не меняется.
            if((new_letter == "" || new_letter == string({ word[position] })) &&
               !visited[new_position][new_v]) {
                result = max(result, FindFirstBad(new_v, new_position, word, visited));
            }
        }
        return result;
    }
    bool ParsablePostfixRegex (const string &regex) {
        /** Простая проверка корректности ввода
         */
        int stack_size = 0;
        int error = 0;
        for (int i = 0; i < regex.length(); ++i) {
            char current_char = regex[i];
            switch (current_char) {
                case '1':
                case 'a':
                case 'b':
                case 'c':
                    ++stack_size;
                    break;
                case '*':
                    if (stack_size == 0) {
                        error = 1;
                    }
                    break;
                case '+':
                case '.':
                    if (stack_size < 2) {
                        error = 1;
                    }
                    --stack_size;
                    break;
                default:
                    error = 1;
            }
            if (error) {
                return false;
            }
        }
        return (stack_size == 1);
    }
public:
    AutomatonBuilder() {}
    AutomatonBuilder(char x):transitions_(vector<vector<Edge> >(2)), terminals_(vector<bool>(2)) {
        if (x == '1') {
            vertices_num_ = 1;
            terminals_.pop_back();
            terminals_[0] = true;
        } else {
            vertices_num_ = 2;
            string s = "";
            s += x;
            transitions_[0].push_back(Edge(1, s));
            terminals_[0] = false;
            terminals_[1] = true;
        }
    }
    AutomatonBuilder(string s) {
        if (!ParsablePostfixRegex(s)) {
            std::cout << "Wrong input format" << endl;
            vertices_num_ = 0;
        } else {
            stack<AutomatonBuilder> st;
            for (int i = 0; i < s.size(); ++i) {
                if (s[i] == 'a' || s[i] == 'b' || s[i] == 'c' || s[i] == '1') {
                    st.push(AutomatonBuilder(s[i]));
                } else if (s[i] == '*') {
                    AutomatonBuilder temp((st.top()).Star());
                    st.pop();
                    st.push(temp);
                } else if (s[i] == '+') {
                    AutomatonBuilder temp(st.top());
                    st.pop();
                    AutomatonBuilder temp2(st.top());
                    st.pop();
                    st.push(temp2 + temp);
                } else if (s[i] == '.') {
                    AutomatonBuilder temp(st.top());
                    st.pop();
                    AutomatonBuilder temp2(st.top());
                    st.pop();
                    st.push(temp2 * temp);
                }
            }
            AutomatonBuilder other = st.top();
            vertices_num_ = other.vertices_num_;
            transitions_ = other.transitions_;
            terminals_ = other.terminals_;
            //    print();
            FormSubstringsAutomaton();
        }

    }

    AutomatonBuilder (unsigned int vertices_num, const vector< vector<Edge> >& transitions, vector<bool> terminals):
            vertices_num_(vertices_num),
            transitions_(transitions),
            terminals_(terminals) {}
    AutomatonBuilder (const AutomatonBuilder& other) {
        vertices_num_ = other.vertices_num_;
        transitions_ = other.transitions_;
        terminals_ = other.terminals_;
    }

    AutomatonBuilder operator + (const AutomatonBuilder& other) const {
        /** Cоздадим новую вершину, из которой проведем эпсилон-ребра в начальные состояния двух автоматов,
        * остальные ребра менять не будем. Добавятся два ребра.
        */
        std::size_t new_vertices_num = vertices_num_ + other.vertices_num_ + 1;
        vector< vector<Edge> > new_transitions (new_vertices_num);
        vector<bool> new_terminals(new_vertices_num);

        new_transitions[0].push_back(Edge(1, ""));
        new_transitions[0].push_back(Edge(vertices_num_ + 1, ""));

        for (int i = 0; i < vertices_num_; ++i) {
            for (int j = 0; j < transitions_[i].size(); ++j) {
                int new_v = transitions_[i][j].v_ + 1;
                string new_letter = transitions_[i][j].letter_;
                new_transitions[i + 1].push_back(Edge(new_v, new_letter));
            }
            new_terminals[i + 1] = terminals_[i];
        }
        for (int i = 0; i < other.vertices_num_; ++i) {
            int index = vertices_num_ + i + 1;
            for (int j = 0; j < other.transitions_[i].size(); ++j) {
                int new_v = other.transitions_[i][j].v_ + vertices_num_ + 1;
                string new_letter = other.transitions_[i][j].letter_;
                new_transitions[index].push_back(Edge(new_v, new_letter));
            }
            new_terminals[index] = other.terminals_[i];
        }

        return AutomatonBuilder(new_vertices_num, new_transitions, new_terminals);
    }

    AutomatonBuilder operator * (const AutomatonBuilder& other) const {
        /** Из завершающего состояния первого автомата
         * проведем эпсилон-ребро в начальное состояние второго.
         * Снимем с завершающего состояния первого автомата пометку
         * того, что оно завершающее.
         */
        size_t new_vertices_num = vertices_num_ + other.vertices_num_;
        vector< vector<Edge> > new_transitions (new_vertices_num);
        vector<bool> new_terminals(new_vertices_num);

        for (int i = 0; i < vertices_num_; ++i) {
            for (int j = 0; j < transitions_[i].size(); ++j) {
                new_transitions[i].push_back(Edge(transitions_[i][j].v_, transitions_[i][j].letter_));
            }
            new_terminals[i] = false;
            if (terminals_[i]) {
                new_transitions[i].push_back(Edge(vertices_num_, ""));
            }
        }
        for (int i = 0; i < other.vertices_num_; ++i) {
            for (int j = 0; j < other.transitions_[i].size(); ++j) {
                new_transitions[vertices_num_ + i].push_back(Edge(other.transitions_[i][j].v_ + vertices_num_, other.transitions_[i][j].letter_));
            }
            new_terminals[vertices_num_ + i] = other.terminals_[i];
        }
        return AutomatonBuilder(new_vertices_num, new_transitions, new_terminals);
    }

    AutomatonBuilder Star() const {
        /** Добавим новую стартовую вершину, из которой проведем эпсилон-переход в стартовую вершину
         * исходного автомата. Пометим новую стартовую вершину вершину завершающей, все старые вершины
         * сдклаем незавершающими. Если старая вершина - завершающая в исходном автомате, проведем из нее эпсилон-ребро
         * в новую стартовую вершину.
         */
        size_t new_vertices_num = vertices_num_ + 1;
        vector< vector<Edge> > new_transitions (new_vertices_num);
        vector<bool> new_terminals(new_vertices_num);
        new_terminals[0] = true;
        new_transitions[0].push_back(Edge(1, ""));

        for (int i = 0; i < vertices_num_; ++i) {
            for (int j = 0; j < transitions_[i].size(); ++j) {
                new_transitions[i + 1].push_back(Edge(transitions_[i][j].v_ + 1, transitions_[i][j].letter_));
            }
            new_terminals[i + 1] = false;
            if (terminals_[i]) {
                new_transitions[i + 1].push_back(Edge(0, ""));
            }
        }
        return AutomatonBuilder(new_vertices_num, new_transitions, new_terminals);
    }

    void print() const {
        std::cout << "Number of conditions: " << vertices_num_ << endl;
        std::cout << "Finals: ";
        for (int i = 0; i < vertices_num_; ++i) {
            if (terminals_[i]) {
                std::cout << i << ' ';
            }
        }
        std::cout << std::endl;
        std::cout << "transitions_ sizes: ";
        for (int i = 0; i < vertices_num_; ++i) {
            std::cout << transitions_[i].size() << ' ';
        }
        std::cout << std::endl;
        for (int i = 0; i < vertices_num_; ++i) {
            for (int j = 0; j < transitions_[i].size(); ++j) {
                std::cout << i << " --> " << transitions_[i][j].v_ << "(" << transitions_[i][j].letter_ << ")" << std::endl;
            }
        }
    }

    void FormSubstringsAutomaton() {
        /** Создаем новую начальнцю и конечную вершину. Из всех вершин исходного автомата проведем эпсилон ребра
         * в новую завершающую вершину и из новой старотовой проведем эпсилон переходы во все исходные вершины
         */
        size_t new_vertices_num = vertices_num_ + 2;
        vector<bool> new_terminals(new_vertices_num, false);
        new_terminals[new_vertices_num - 1] = true;
        vector< vector<Edge> >  new_transitions(new_vertices_num);
        for (int i = 0; i < vertices_num_; ++i) {
            new_transitions[0].push_back(Edge(i + 1, ""));
            new_transitions[i + 1].push_back(Edge(new_vertices_num - 1, ""));
        }
        for (int i = 0; i < vertices_num_; ++i) {
            for (int j = 0; j < transitions_[i].size(); ++j) {
                int new_v = transitions_[i][j].v_ + 1;
                string new_letter = transitions_[i][j].letter_;
                new_transitions[i + 1].push_back(Edge(new_v, new_letter));
            }
        }
        terminals_ = new_terminals;
        transitions_ = new_transitions;
        vertices_num_= new_vertices_num;
    }
    int ProcessWord(const string &word_to_process);

};
int main() {
    string s, pattern;
    std::cin >> s >> pattern;
    AutomatonBuilder D(s);
    int answer = D.ProcessWord(pattern);
    std::cout << "answer is " << answer << std::endl;
}
//acb..bab.c.*.ab.ba.+.+*a. cbaa
//ab+c.aba.*.bac.+.+* babc
int AutomatonBuilder::ProcessWord(const string &word_to_process) {
    /** Функция для нахождения ответа. Осуществляется дфс-ом FindFirstBad(node, text_position, word, visited
     * по ходу выполнения обновляем результат max_length
     * visited[text_position][state_index] хранит true/false в зависимости от того, пришли ли мы в состояние под номером
     * state_index, запустив слово с позиции text_position. Это помогает не заходить в циклы из эпсилон ребер
     * text_position обновляется если переход осуществлен по текущей букве word[text_position] слова word
     */
    if (vertices_num_ == 0) {
        std::cout << "Wrong input" << std::endl;
        return -1;
    }
    int max_length = 0;
    size_t word_length = word_to_process.length();
    for (int s = 0; s < word_length; ++s) {
        vector <bool> visited_nodes(vertices_num_, false);
        vector < vector<bool> > visited;
        visited.assign(word_length + 1, visited_nodes);
        // Заполним все сразу
        for (int k = 0; k <= word_length; ++k) {
            for (int i = 0; i < vertices_num_; ++i) {
                visited[k][i] = false;
            }
        }
        int temp_res = FindFirstBad(0, s, word_to_process, visited) - s;
        max_length = max(max_length, temp_res);
        //    cout << "ml" << max_length;
    }

    return max_length;
}
