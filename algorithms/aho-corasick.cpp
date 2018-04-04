/* Шаблон поиска задан строкой длины m, в которой кроме обычных символов могут встречаться символы “?”.
 * Найти позиции всех вхождений шаблона в тексте длины n.
 * Каждое вхождение шаблона предполагает, что все обычные символы совпадают с соответствующими из текста,
 * а вместо символа “?” в тексте встречается произвольный символ.
 * Время работы - O(n + m + Z), где Z - общее -число
 * вхождений подстрок шаблона “между вопросиками” в исходном тексте.
 */
#include <array>
#include <iostream>
#include <string>
#include <vector>


struct Node {
    std::vector<int> next_states_;
    std::vector<int> delta_;
    std::vector<int> lines_;
    bool is_leaf_;
    int parent_;
    int state_by_char_;
    int suf_link_;
    int up_link_;
    explicit Node(int n) : is_leaf_(false), parent_(0), state_by_char_(0), suf_link_(-1), up_link_(-1) {
        next_states_.assign(n, -1);
        delta_.assign(n, -1);
    };
};

class AhoCorasick {
public:
    explicit AhoCorasick(const std::string & pattern) {
        last_ = 0;
        power_ = 26;
        Prepare(pattern);
        pattern_length_ = pattern.length();
        Node root(power_);
        nodes_.push_back(root);

        last_ = 1;
        for (int i = 0; i < string_vector.size(); ++i) {
            AddString(string_vector[i], i);
        }
    }

    void StringSearch(const std::string& text);
    void PrintAnswer() {
        for (int i : answer_)
            std::cout << i << " ";
    }

private:
    std::vector <std::string> string_vector;
    std::vector <int> starts_;
    std::vector<Node> nodes_;
    std::vector<int> answer_;
    int power_;
    int pattern_length_;
    unsigned int last_;

    void Prepare(const std::string & text);
    void AddString(const std::string & text, int ind);
    int GetSuffLink(int state);
    int GetUpLink(int state);
    int GetDelta(int state, int a);
};

int main() {
    std::string pattern;
    std::cin >> pattern;

    AhoCorasick example(pattern);

    std::string text;
    std::cin >> text;
    example.StringSearch(text);
    example.PrintAnswer();
    return 0;
}

void AhoCorasick::StringSearch(const std::string &text) {
    if (text.length() < pattern_length_)
        return;

    int i = 0;
    std::vector<int> solver(text.length());

    for (int character = 0; character < text.length(); ++character) {
        auto t = static_cast<int> (text[character]- 'a');
        i = GetDelta(i, t);

        int temporary_ind = i;
        while (temporary_ind != 0) {
            if (nodes_[temporary_ind].is_leaf_) {
                for (int line : nodes_[temporary_ind].lines_) {
                    if (character - line + 1 >= 0) {
                        ++solver[character - line + 1];
                    }
                }
            }
            temporary_ind = GetUpLink(temporary_ind);
        }
    }

    for (int j = 0; j < solver.size() - pattern_length_ + 1; ++j) {
        if (solver[j] == string_vector.size())
            answer_.push_back(j);
    }
}

void AhoCorasick::Prepare(const std::string &text) {
    int i = 0;
    while (i < text.length()) {
        std::string current;
        while (text[i] == '?'&& i < text.length())
            ++i;
        if (i < text.length()) {
            while (text[i] != '?' && i < text.length()) {
                current += text[i];
                ++i;
            }
            string_vector.push_back(current);
            starts_.push_back(i);
        }
    }
}

void AhoCorasick::AddString(const std::string &text, int ind) {
    int state = 0;
    for (char character : text) {
        int temporary_state;
        temporary_state = static_cast<int>(character - 'a');
        if (nodes_[state].next_states_[temporary_state] == -1) {
            Node node(power_);
            node.suf_link_ = -1;
            node.parent_ = state;
            node.state_by_char_ = temporary_state;
            nodes_[state].next_states_[temporary_state] = last_;
            nodes_.push_back(node);
            ++last_;
        }

        state = nodes_[state].next_states_[temporary_state];
    }


    nodes_[state].is_leaf_ = true;
    nodes_[state].lines_.push_back(starts_[ind]);
}

int AhoCorasick::GetSuffLink(int state) {
    if (nodes_[state].suf_link_ == -1) {
        int par = nodes_[state].parent_, sbc = nodes_[state].state_by_char_;
        if (state == 0 || par == 0) {
            nodes_[state].suf_link_ = 0;
        } else {
            nodes_[state].suf_link_ = GetDelta(GetSuffLink(par), sbc);
        }
    }
    return nodes_[state].suf_link_;
}

int AhoCorasick::GetDelta(int state, int a) {
    if (nodes_[state].delta_[a] == -1)
        if (nodes_[state].next_states_[a] != -1) {
            nodes_[state].delta_[a] = nodes_[state].next_states_[a];
        } else {
            nodes_[state].delta_[a] = (state == 0) ? 0 : GetDelta(GetSuffLink(state), a);
        }

    return nodes_[state].delta_[a];
}

int AhoCorasick::GetUpLink(int state) {
    if (nodes_[state].up_link_ == -1) {
        int temporary_state = GetSuffLink(state);
        if (nodes_[temporary_state].is_leaf_) {
            nodes_[state].up_link_ = temporary_state;
        } else {
            nodes_[state].up_link_ = temporary_state == 0 ? 0 : GetUpLink(temporary_state);
        }
    }
    return  nodes_[state].up_link_;
}
