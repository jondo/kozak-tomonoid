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
  if (ptrset->size() == remap.size() )
  {
    // No need for rebuilding, no (new) strongly connected components found
    for (auto it = remap.begin(); it != remap.end(); ++it)
    {
      std::set<vertex*> *tbmerged = (*it).second;
      delete tbmerged;
    }
    return;
  }
  // 1) Move all Table Elements to just one associated set in each component
  // Delete all other vertices
  for (auto it = remap.begin(); it != remap.end(); ++it)
  {
    vertex *joined = (*it).first;
    std::set<vertex*> *tbmerged = (*it).second;
    
    std::set<vertex*> *jPrec = (*(precededSets->find(joined))).second;
    std::set<vertex*> *jRev = (*(revertSets->find(joined))).second;
    
    for (auto inner = tbmerged->begin(); inner != tbmerged->end(); ++inner)
    {
      vertex *next = *inner;
      if (next != joined)
      {
	// move all table elements to joined set
	joined->insert(next->begin(), next->end());
	// delete all entries of this vertex
	ptrset->erase(next);
	auto bla = precededSets->find(next);
	std::set<vertex*> *nPrec = (*bla).second;
	jPrec->insert(nPrec->begin(), nPrec->end());
	delete nPrec;
	precededSets->erase(next);
	
	auto bla2 = revertSets->find(next);
	std::set<vertex*> *nRev = (*bla2).second;
	jRev->insert(nRev->begin(), nRev->end() );
	delete nRev;
	revertSets->erase(next);
	
	delete next;
      }
    }
    delete tbmerged;
  }
  // 2) Change ALL preceded and revertSets...
  renameValues(precededSets);
  renameValues(revertSets);
}

void Tomonoid::StrongConnectivityFinder::renameValues(std::map<vertex*,std::set<vertex*>*> *curr_set)
{
  for (auto it = curr_set->begin(); it != curr_set->end(); ++it)
  {
    vertex *inVert = (*it).first;
    std::set<vertex*> *inSet = (*it).second;
    std::set<vertex*> toDel, toIns;
    for (auto inner = inSet->begin(); inner != inSet->end(); ++inner)
    {
      vertex *ptr = *inner;
      auto ncIt = newClass.find(ptr);
      if (ncIt != newClass.end() )
      {
	toDel.insert(ptr);
	toIns.insert((*ncIt).second);
      }
    }
    
    for (auto inner = toDel.begin(); inner != toDel.end(); ++inner)
    {
      inSet->erase(*inner);
    }
    for (auto inner = toIns.begin(); inner != toIns.end(); ++inner)
    {
      vertex *ptr = *inner;
      if (ptr != (*it).first)
      {
	inSet->insert(*inner);
      }
    }
    inSet->erase(inVert);
  }
}

void Tomonoid::StrongConnectivityFinder::tarjan()
{
  int index = 0;
  
#ifdef VERBOSE
  std::cout << "tarjan" << std::endl;
#endif
  
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

int Tomonoid::StrongConnectivityFinder::strongConnect(Tomonoid::StrongConnectivityFinder::vertex* v)
{
  int curr_index = index;
  indices.insert(std::make_pair(v, index));
  int lowlink = index;
  ++index;
  onStack.insert(v);
  stack.push(v);
  
#ifdef VERBOSE
  std::cerr << "Now checking: " << v << ", index: " << curr_index << std::endl;
#endif
  
  auto prit = precededSets->find(v);
  std::set<vertex*> *precs = (*prit).second;
  
  for (auto it = precs->begin(); it != precs->end(); ++it)
  {
    vertex *next = *it;
#ifdef VERBOSE
    std::cerr << "Iterated to " << next << std::endl;
#endif
    if (indices.find(next) == indices.end() )
    {
#ifdef VERBOSE
      std::cerr << next << " doesn't have index, continue here" << std::endl;
#endif
      int nextLow = strongConnect(next);
      lowlink = std::min(nextLow, lowlink);
    }
    else if (onStack.find(next) != onStack.end() )
    {

      auto nit = indices.find(next);
      int nextIndex = (*nit).second;
#ifdef VERBOSE
      std::cerr << next << " on stack, check lowlink change" << std::endl;
      std::cerr << "curr.lowlink: " << lowlink << ", next index: " << nextIndex << std::endl;
#endif
      lowlink = std::min(lowlink, nextIndex);
#ifdef VERBOSE
      std::cerr << "curr.lowlink = " << lowlink << std::endl;
#endif
    }
  }
  
  if (lowlink == curr_index)
  {
    vertex *inComp = NULL;
    std::set<vertex*> *helper = new std::set<vertex*>(); 
#ifdef VERBOSE
    std::cerr << "Next strongly connected component consists of:" << std::endl;
#endif
    do 
    {
      inComp = stack.top();
      stack.pop();
      onStack.erase(inComp);
      helper->insert(inComp);
      if (inComp != v) 
      {
	newClass.insert(std::make_pair<>(inComp, v));
      }
#ifdef VERBOSE
      std::cerr << inComp << std::endl;
#endif
    }
    while (inComp != v);
    remap.insert(std::make_pair(v, helper));
  }
  
  return lowlink;
  
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