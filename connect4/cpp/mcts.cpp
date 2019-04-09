#include "mcts.h"

const double EXPLOR_PARAM = 1.3;//0.70710678118; //1.41421356237 //0.52026009502
const int depth_max = 50;

MCTS::MCTS(int time_limit, bool first_player){
  srand(42);

  this->time_limit = time_limit;
  this->first_player = first_player;
}

void MCTS::init(Board *board){
  clean(root);

  this->root = new Node(NULL, this->first_player, board->dup());
  expand(this->root);
  this->board = board;
}

void MCTS::play(Play p){
  Node *child;
  int i = 0;

  //see wich child is now root
  for(Play x: root->lst_plays){
    if(x == p) break;
    i++;
  }

  //if child not found
  if(i == root->lst_plays.size()){
    printf("MCTS::play play not found %d\n",i);
    exit(1);
  }

  //save child and clean root
  child = root->lst_childs[i];
  root->lst_childs[i] = NULL;
  clean(root);

  //root is now child
  root = child;
  root->parent = NULL;

  if(!root->has_childs()) expand(root);

}

void MCTS::search(){
  clock_t start_time = clock();

  Node *child;
  Board *dup;
  int res;

  int jogos = 0;
  //while((clock() - start_time) < time_limit){
  while(jogos<250000){
    child = select(root);
    if(!child->terminal) expand(child);

    for(int i=0;i<1;i++){
      //simulate
      dup = child->board->dup();

      if(child->terminal) res = child->res;
      else res = simulate(dup,child->next_player,depth_max,child);
      //backpropagate
      backpropagate(child, res);

      delete(dup);
      jogos++;
    }

  }

  int best_i = 0;
  double best_val = -10000.0,val;
  int sz = (int)root->lst_childs.size();

  for(int i=0;i<sz;i++){
    child = root->lst_childs[i];
    val = child->games;
    if(val > best_val){
        best_i = i;
        best_val = val;
    }
    printf("%.2lf %.2lf %d\n",eval(root, child,0), child->reward, child->games);//printPlay(root->lst_plays[i]);
  }

  board->best_play = root->lst_plays[best_i];

}


double MCTS::eval(Node *parent,Node *node, double EXPLOR_PARAM){
  if(!node->has_childs()){
    return 0.5 + EXPLOR_PARAM*sqrt(log(parent->games+1));
  }

  double wr = node->reward/(node->games+1);
  if(root->next_player!=node->next_player) wr = 1.0-wr;
  return wr + EXPLOR_PARAM*sqrt(log(parent->games+1)/((double)node->games+1));
}

int MCTS::select_child(Node* node){
  int sz = (int)node->lst_childs.size();

  std::vector<int> v;

  if(sz != node->nexpanded){
      for(int i=0;i<sz;i++){
        if(!node->lst_childs[i]->has_childs()) v.push_back(i);
      }
      std::random_shuffle(v.begin(),v.end());
      node->nexpanded++;
      return v[0];
  }

  double best_value = -100000.0,val;
  Node *child;

  for(int i=0;i<sz;i++){
    child = node->lst_childs[i];

    val = eval(node, child, EXPLOR_PARAM);
    if(val>best_value){
      best_value = val;
      v.clear();
      v.push_back(i);
    }
    else if(val==best_value) v.push_back(i);

    //printf("%.2lf %.2lf %d\n",val, child->reward, child->games);//printPlay(root->lst_plays[i]);
  }
  std::random_shuffle(v.begin(),v.end());
  return v[0];
}

Node* MCTS::select(Node *node){
  if(!node){
    printf("Erro null\n");
    exit(1);
  }
  if(!node->has_childs()) return node;

  int best = select_child(node);
  return select(node->lst_childs[best]);
}

void MCTS::expand(Node *node){
  Board *dup;

  node->lst_plays = node->board->getPlays(node->next_player);
  for(int i=0;i<(int)node->lst_plays.size();i++){
    dup = node->board->dup();
    dup->play(node->lst_plays[i]);
    node->lst_childs.push_back(new Node(node,!node->next_player,dup));
  }
}

int MCTS::simulate(Board *board, bool player1, int depth, Node *child){
  if(board->gameOver(player1)){
    int k = board->whoWins(player1);
    if(depth == depth_max){
      child->terminal = true;
      child->res = k;
    }
    return k;
  }
  if(!depth) return 0;

  auto plays = board->getPlays(player1);

  //random play
  std::random_shuffle(plays.begin(), plays.end());
  Play p = plays[0];

  board->play(p);

  return simulate(board,!player1,depth-1,child);
}

void MCTS::backpropagate_aux(Node *node, double val, bool player){
  if(!node) return;

  if(node->next_player==player) node->reward+=val;
  node->games+=1;

  backpropagate_aux(node->parent,val, player);
}

void MCTS::backpropagate(Node *node, int res){
  if((node->next_player && res>0) || (!node->next_player && res<0)){
    backpropagate_aux(node, 1.0, node->next_player);
  }
  else if((node->next_player && res<0) || (!node->next_player && res>0)){
    backpropagate_aux(node, 0, node->next_player);
  }
  else if(!res){
    backpropagate_aux(node, 0, node->next_player);
  }
}

/*
void MCTS::backpropagate_aux(Node *node, double val){
  if(!node) return;

  node->reward+=val;
  node->games+=1;

  backpropagate_aux(node->parent,val);
}

void MCTS::backpropagate(Node *node, int res){
  if((root->next_player && res>0) || (!root->next_player && res<0)){
    if(node->terminal)
      backpropagate_aux(node, 20.0);
    else
      backpropagate_aux(node, 1.0);
  }
  else if((root->next_player && res<0) || (!root->next_player && res>0)){
    if(node->terminal)
      backpropagate_aux(node, -30.0);
    else
      backpropagate_aux(node, -1.0);
  }
  else if(!res){
    backpropagate_aux(node, 0);
  }
}
*/
