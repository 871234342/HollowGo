#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include <fstream>
#include <cmath>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

std::default_random_engine engine;

bool time_up = false;

char t;

void mcts_timeout(int sig) {
    time_up = true;
    // std::cout << "alarm triggered\n";
}

void child_handler(int sig) {
    pid_t pid;
    int status;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // std::cout << "child " << pid << "terminated\n";
        continue;
    }
}

int space[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
9, 10, 11, 12, 13, 14, 15, 16, 17,
18, 19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34, 35,
36, 37, 38, 39, 40, 41, 42, 43, 44,
45, 46, 47, 48, 49, 50, 51, 52, 53,
54, 55, 56, 57, 58, 59, 60, 61, 62,
63, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 80};

struct placement{
    /**
     * "who" places at "pos"
     */
    int pos;
    board::piece_type who;

    placement(int p, board::piece_type w) : pos(p), who(w) {}
};

struct sim_result{
    bool shyoubu;
    std::vector<placement> katei;

    sim_result() {
        katei.clear();
    }
};

class node{
public:
    node(board new_board, board::piece_type player_type){
        // init
        who = player_type;
        if (who == board::black)    child_type = board::white;
        else                        child_type = board::black;
        current = new_board;
    }

    ~node() {
        for (node* child : children) {
            delete child;
        }
    }

    /**
     * return the child node with highest UCB, or itself if the node has unexplored child
     */
    node* select(board::piece_type root_type, double RAVE) {
        if (num_of_child != explored_child || num_of_child == 0)     return this;
        double best_value = 0;
        double c = 0.7;
        node* best=NULL;
        for (node* child : children) {
            double value;
            if (root_type == who)   
                value = (1 - RAVE) * child->Q + RAVE * child->Q_RAVE + std::sqrt(std::log(N) / child->N) * c;
            else
                value = (1 - RAVE) * (1 - child->Q) + RAVE * (1 - child->Q_RAVE) + std::sqrt(std::log(N) / child->N) * c;
            if (value >= best_value) {
                best_value = value;
                best = child;
            }
        }
        return best;
    }

    /**
     * return the node expanded to simulate
     * if is leaf (num_of_child == 0), create all children first before return
     * if the node is terminal, return itself
     */
    node* expand() {
        if (num_of_child != 0 && explored_child == num_of_child) {
            std::cout<<"WTF?";
            exit(1);
        }
        if (terminated) {
            return this;
        }
        else if (num_of_child == 0) {
            // generate all children
            for (int pos : space) {
                board after = current;
                action::place move = action::place(pos, who);
                if (move.apply(after) == board::legal) {
                    node* child = new node(after, child_type);
                    child->parent = this;
                    child->parent_move = pos;
                    children.push_back(child);
                }
            }
            num_of_child = children.size();

            if (num_of_child == 0) {
                terminated = true;
                return this;
            }
            return children[explored_child++];
        }
        else {
            return children[explored_child++];
        }
    }

    /**
     * return true if win for root peice type
     * both player play randomly
     */ 
    sim_result simulate(board::piece_type root_player) {
        board simulate = current;
        board::piece_type current_player = who;
        bool checkmate = true;
        sim_result kekka;
        for (;;) {
            std::shuffle(&space[0], &space[81], engine);
            for (int pos : space) {
                action::place move = action::place(pos, current_player);
                if (move.apply(simulate) == board::legal) {
                    checkmate = false;
                    kekka.katei.emplace_back(placement(pos, current_player));
                    break;
                }
            }
            if (checkmate) {
                kekka.shyoubu = (current_player != root_player);
                return kekka;
            }
            else {
                checkmate = true;
                current_player = (current_player == board::black ? board::white : board::black);
            }
        }
    }

    /**
     * return the parent node to update
     */
    node* update(bool victory) {
        N++;
        Q += (victory - Q) / N;
        return parent;
    }

    void RAVE_update(bool victory) {
        N_RAVE++;
        Q_RAVE += (victory - Q_RAVE) / N_RAVE;
    }

    action::place best_action(int *count_table = NULL) {
        int most_visit_count = 0;
        int best_move = -1;
        if (count_table == NULL) {
            for (node* child : children) {
                int visit_count = child->N;
                if (visit_count >= most_visit_count) {
                    most_visit_count = visit_count;
                    best_move = child->parent_move;
                } 
            }
        }
        else {
            for (node* child : children) {
                int visit_count = child->N;
                count_table[child->parent_move] += child->N;
                if (visit_count >= most_visit_count) {
                    most_visit_count = visit_count;
                    best_move = child->parent_move;
                }
            }
        }

        return action::place(best_move, who);
    }

public:
    board current;
    int num_of_child = 0, explored_child = 0;
    board::piece_type who;  //type to play next
    board::piece_type child_type;
    node* parent = NULL;
    std::vector<node*> children;
    bool terminated = false;
    int N = 0, N_RAVE = 0, parent_move = -1;
    double Q = 0, Q_RAVE = 0;
};


class mcts{
public:
    mcts(const board& root_board, board::piece_type player_type, int c, int t, double r = 0) : 
        root(root_board, player_type), cycles(c), think_time(t), RAVE(r) {
            path.clear();
        }

    node* select() {
        node* selecting = &root;
        node* next;
        while ((next = selecting->select(root.who, RAVE)) != selecting) {
            selecting = next;
            path.emplace_back(placement(selecting->parent_move, selecting->parent->who));
        }
        return selecting;
    }

    node* expand(node* to_expand) {
        node* to_sim = to_expand->expand();
        if (to_sim != to_expand) {
            path.emplace_back(placement(to_sim->parent_move, to_sim->parent->who));
        }
        return to_sim;
    }

    sim_result simulate(node* to_simulate) {
        return to_simulate->simulate(root.who);
    }

    void update(node* leaf, bool win) {
        while((leaf = leaf->update(win)) != NULL) {
            continue;
        }
    }

    void traverse(bool win, node* start) {
        while(start != NULL) {
            for (node* child : start->children) {
                for (placement move : path) {
                    if (child->parent_move == move.pos && child->parent->who == move.who) {
                        child->RAVE_update(win);
                    }
                }
                for (placement move : mogi.katei) {
                    if (child->parent_move == move.pos && child->parent->who == move.who) {
                        child->RAVE_update(win);
                    }
                }
            }
            start = start->parent;
        }
    }

    action::place tree_search(int *table = NULL) {
        if (cycles != 0) {
            if (RAVE != 0) {
                for (int i = 0; i < cycles; i++) {
                    path.clear();
                    mogi.katei.clear();
                    node* working = this->select();
                    working = expand(working);
                    mogi = simulate(working);
                    update(working, mogi.shyoubu);
                    traverse(mogi.shyoubu, working);
                }
            }
            else {
                for (int i = 0; i < cycles; i++) {
                    node* working = this->select();
                    working = expand(working);
                    mogi = simulate(working);
                    update(working, mogi.shyoubu);
                }
            }
        }
        else {
            time_up = false;
            signal(SIGALRM, &mcts_timeout);
            // std::cout << "alarm will be triggered after " << think_time <<" ms\n";
            timeval tv_interval = {0, 0};
            timeval tv_value = {think_time / 1000, (think_time % 1000) * 1000};
            itimerval it;
            it.it_interval = tv_interval;
            it.it_value = tv_value;
            if(setitimer(ITIMER_REAL, &it, NULL) != 0) {
                perror("settimer");
            }
            if (RAVE != 0) {
                while(1) {
                    path.clear();
                    node* working = this->select();
                    working = expand(working);
                    mogi = simulate(working);
                    update(working, mogi.shyoubu);
                    traverse(mogi.shyoubu, working);
                    if (time_up)   break;
                }
            }
            else {
                while(1) {
                    node* working = this->select();
                    working = expand(working);
                    mogi = simulate(working);
                    update(working, mogi.shyoubu);
                    if (time_up)   break;
                }
            }
        }
        /////////////////////////
        // int total = 0;
        // std::cout << "ONE PROCESS:\n";
        // for (int i = 8; i >= 0; i--) {
        //     for (int j = 8; j >= 0; j--) {
        //         bool found = false;
        //         for (node* child : root.children) {
        //             if (child->parent_move == i * 9 + j) {
        //                 std::cout << child->N << '\t';
        //                 total += child->N;
        //                 found = true;
        //                 break;
        //             }
        //         }
        //         if (!found) {
        //             std::cout << 0 << '\t';
        //         }
        //     }
        //     std::cout << '\n';
        // }
        // std::cout << root.current << '\n';
        // std::cout << "total: " << total << "\n\n";

        /////////////////////////

        return root.best_action(table);
    }

    action::place tree_search_parallel(int parallel) {
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            return tree_search();
        }
        pid_t pid;
        signal(SIGCHLD, child_handler);
        for (int i = 1; i < parallel; i++) {
            pid = fork();
            if (pid == 0) {
                break;
            }
            else if (pid < 0) {
                perror("fork");
                std::cin >> t;
            }
        }
        
        if (pid == 0) { // child
            close(pipefd[0]);
            int count_table[81] = {0};
            tree_search(count_table);
            int n = write(pipefd[1], count_table, sizeof(count_table));
            if (n < 0) {
                perror("write");
            }
            close(pipefd[1]);
            exit(0);
        }
        else {          // parent
            close(pipefd[1]);
            int count_table[81] = {0};
            int child_table[81];
            tree_search(count_table);
            for (int i = 1; i < parallel; i++) {
                int n = read(pipefd[0], child_table, sizeof(child_table));
                if (n < 0) {
                    perror("read");
                }
                for (int j = 0; j < 81; j++) {
                    count_table[j] += child_table[j];
                }
            }
            close(pipefd[0]);
            
            int best_move = -1;
            int most_visited = 0;
            for (int move = 0; move < 81; move++) {
                if (count_table[move] > most_visited) {
                    best_move = move;
                    most_visited = count_table[move];
                }
            }
            ////////////////////
            // std::cout <<"TOTAL:\n";
            // int total = 0;
            // for (int i = 8; i >= 0; i--) {
            //     for (int j = 8; j >= 0; j--) {
            //         std::cout << count_table[9 * i + j] << '\t';
            //         total += count_table[9 * i + j];
            //     }
            //     std::cout << '\n';
            // }
            // std::cout << root.current << '\n';
            // std::cout << "best move: " << best_move << "   total: " << total << '\n';
            // if (total != 200) {
            //     std::cin >> t;
            // }
            /////////////////
            return action::place(best_move, root.who);
        }
    }

private:
    node root;
    int cycles;     // number of simulations
    int think_time; // thinking_time in milisecond;
    double RAVE = 0;
    std::vector<placement> path;
    sim_result mogi;
    int parallel;
};