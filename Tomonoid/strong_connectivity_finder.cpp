#include "tomonoid.h"

Tomonoid::StrongConnectivityFinder::StrongConnectivityFinder(std::set< vertex* > *ptrset,
			     std::map< vertex*, std::set< vertex* >* > *precededSets,
			     std::map< vertex*, std::set< vertex* >* > *revertSets)
{
  this->ptrset = ptrset;
  this->precededSets = precededSets;
  this->revertSets = revertSets;
}

void Tomonoid::StrongConnectivityFinder::findComponents()
{
  tarjan();
  rebuildVerticesSets();
}

void Tomonoid::StrongConnectivityFinder::rebuildVerticesSets()
{
  // do da work
}

void Tomonoid::StrongConnectivityFinder::tarjan()
{
  int index = 0;
  
  for (auto it = ptrset->begin(); it != ptrset->end(); ++it)
  {
    vertex *vertex = *it;
    if (indices.find(vertex) == indices.end())
    {
      strongConnect(vertex);
    }
  }
  /* algorithm tarjan is
  input: graph G = (V, E)
  output: set of strongly connected components (sets of vertices)

  index := 0
  S := empty array
  for each v in V do
    if (v.index is undefined) then
      strongconnect(v)
    end if
  end for

  */
}

void Tomonoid::StrongConnectivityFinder::strongConnect(Tomonoid::StrongConnectivityFinder::vertex* v)
{
  int curr_index = index;
  indices.insert(std::make_pair(v, index));
  int lowlink = index;
  ++index;
  onStack.insert(v);
  stack.push(v);
  
  auto prit = precededSets->find(v);
  std::set<vertex*> *precs = (*prit).second;
  
  for (auto it = precs->begin(); it != precs->end(); ++it)
  {
    vertex *next = *it;
    if (indices.find(next) == indices.end() )
    {
      strongConnect(next);
    }
    else if (onStack.find(next) != onStack.end() )
    {
      auto nit = indices.find(next);
      int nextIndex = (*nit).second;
      lowlink = std::min(lowlink, nextIndex);
    }
  }
  
  if (lowlink == curr_index)
  {
    vertex *inComp = NULL;
#ifdef VERBOSE
    std::cerr << "Next strongly connected component consists of:" << std::endl;
#endif
    do 
    {
      inComp = stack.top();
      stack.pop();
      onStack.erase(inComp);
      remap.insert(std::make_pair(v, inComp));
#ifdef VERBOSE
      std::cerr << inComp << std::endl;
#endif
    }
    while (inComp != v);
  }
  /*function strongconnect(v)
    // Set the depth index for v to the smallest unused index
    v.index := index
    v.lowlink := index
    index := index + 1
    S.push(v)
    v.onStack := true

    // Consider successors of v
    for each (v, w) in E do
      if (w.index is undefined) then
        // Successor w has not yet been visited; recurse on it
        strongconnect(w)
        v.lowlink  := min(v.lowlink, w.lowlink)
      else if (w.onStack) then
        // Successor w is in stack S and hence in the current SCC
        // Note: The next line may look odd - but is correct.
        // It says w.index not w.lowlink; that is deliberate and from the original paper
        v.lowlink  := min(v.lowlink, w.index)
      end if
    end for

    // If v is a root node, pop the stack and generate an SCC
    if (v.lowlink = v.index) then
      start a new strongly connected component
      repeat
        w := S.pop()
        w.onStack := false
        add w to current strongly connected component
      while (w != v)
      output the current strongly connected component
    end if
  end function*/
}