#ifndef PERMUTATION_TRY_H
#define PERMUTATION_TRY_H

#include <iostream>

struct Stackpair
{
  int order;
  bool change;
  
  Stackpair(int o, bool b) {
    order = o;
    change = b;
  }
};

void print_vctr(std::vector<bool>& vctr)
{
  std::vector<bool>::iterator it = vctr.begin();
    for (it; it != vctr.end(); ++it)
    {
      std::cout << *it;
    }
    std::cout << std::endl;
}

void permute_stack(std::vector<bool>& vctr)
{
  std::vector<Stackpair> stack;
  stack.push_back(Stackpair(0, false));
  
  int len = vctr.size();
  while (!stack.empty())
  {
    Stackpair s = stack.back();
    stack.pop_back();
    if (s.order == len)
    {
      print_vctr(vctr);
    } else
    {
      if (s.change)
      {
	bool curr = vctr[s.order];
	vctr[s.order] = !curr;
	stack.push_back(Stackpair(s.order + 1, false));
      }
      else 
      {
	stack.push_back(Stackpair(s.order, true));
	stack.push_back(Stackpair(s.order + 1, false));
      }
    }
  }
}

void permute_inner(std::vector<bool>& vctr, int i)
{
  int l = vctr.size();
  
  if (i == l) {
    print_vctr(vctr);
  } 
  else
  {
    permute_inner(vctr, i + 1);
    bool curr = vctr[i];
    vctr[i] = !curr;
    permute_inner(vctr, i + 1);
  }
  
}

void permute(std::vector<bool>& vctr)
{
  permute_stack(vctr);
  //permute_inner(vctr, 0);
}

void try_method()
{
  std::vector<bool> vctr;
  for (int i = 0; i < 5; i++)
  {
    vctr.push_back(true);
  }
  permute(vctr);
}

#endif