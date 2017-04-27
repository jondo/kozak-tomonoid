#include "tomonoid.h"
#include <algorithm>

struct BetterAssignator {
  std::string bitmask;
  int n, size;
  
  BetterAssignator(int size)
  {
    bitmask = std::string(size, 1);
    n = 0;
    this->size = size;
  }
  
  bool finished()
  {
    return n >= size;
  }
  
  const std::string& getNext()
  {
    if (!std::prev_permutation(bitmask.begin(), bitmask.end()))
    {
	n++;
	#ifdef DEBUG
	std::cout << "Adding n, n = " << n << std::endl;
	#endif
        if (n <= size)
	{
	  bitmask = std::string(size - n, 1);
	  bitmask.resize(size, 0);
	}
    }
    return bitmask;
  }
  
};

struct Assignator {
    bool* steps;
    int size;
    int frees = 1;
    std::vector<int> positions;
    
    Assignator(int size) {
	this->steps = new bool[size]; 
	this->size = size;
	
	positions.push_back(1);
	
	for (int i = 0; i < size; i++)
	{
	  steps[i] = true;
	}
    }
    
    bool finished()
    {
      return frees == size;
    }
    
    void move()
    {
      int res = size;
      move(frees, res);
    }
    
    void move(int order, int resLen)
    {
      int pos = positions[order - 1];
      if (pos < resLen - 1)
      {
	steps[pos] = true;
	pos++;
	steps[pos] = false;
	positions[order - 1] = pos;
      }
      else
      {
	if (order == 1) // add new free element
	{
	  frees++;
	  for (int i = 0; i < frees; i++)
	  {
	    steps[i] = false;
	    positions[i] = i;
	  }
	  for (int j = frees; j < size; j++)
	  {
	    steps[j] =  true;
	  }
	}
	else
	{
	  
	}
      }
    }
    
    std::vector<int> nextPositions()
    {
      std::vector<int> toRet;
      if (finished() ) return toRet;
      move();
      for (int i = 0; i < size; i++)
      {
	if (steps[i]) toRet.push_back(i);
      }
      return toRet;
    }
    
    ~Assignator() {
      delete[] steps;
    }
};


// TODO - tohle fakt musi jit nejak jinak...
void Tomonoid::checkVals(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets,
	       std::vector<Tomonoid*> &res,
	       Tomonoid *primary,
	       std::unordered_map< TableElement, std::set< TableElement >* >& telToSet,
	       std::set<std::set<TableElement>*>& cornerSets,
	       std::set<std::set<TableElement>*>& lockSets
	      )
{
  results_map r1;
  assignRecursively(r1, precededSets, cornerSets, lockSets);
  Tomonoid *tnew = new Tomonoid(*primary);
  tnew->setImportantResults(r1);
  res.push_back(tnew);
  
  int cornsetsize = cornerSets.size();
  
  #ifdef DEBUG
  std::cout << "Cornsize " << cornsetsize << std::endl;
  #endif
  
  if (cornsetsize == 0) return;
  
  BetterAssignator agn(cornsetsize);
  
  std::map<unsigned int, std::set<TableElement>*> positions;
  unsigned int i = 0;
  for (std::set<std::set<TableElement>*>::iterator cit = cornerSets.begin(); cit != cornerSets.end(); ++cit)
  {
    std::set<TableElement> *ptr = *cit;
    positions.insert(std::make_pair(i, ptr));
    i++;
  }
  
  int thisLevelStart = this->leftBeginning;
  
  while (!agn.finished())
  {
    const std::string &ress = agn.getNext();
    std::set<std::set<TableElement>*> nextLockSets(lockSets);
    std::set<std::set<TableElement>*> nextCornerSets;
    std::set<std::set<TableElement>*> prohibitedSets;
    
    for (unsigned int i = 0; i < cornsetsize; i++)
    {
      std::map<unsigned int, std::set<TableElement>*>::iterator omg = positions.find(i);
      std::set<TableElement> *ptrb = (*omg).second;
      #ifdef DEBUG
      for (std::set<TableElement>::iterator c = ptrb->begin(); c != ptrb->end(); c++)
      {
	std::cout << *c << " ";
      }
      #endif
      if (ress[i])
      {
	#ifdef DEBUG
	std::cout << "1" << std::endl;
	#endif
	nextLockSets.insert(ptrb);
      }
      else
      {
	#ifdef DEBUG
	std::cout << "0" << std::endl;
	#endif
	prohibitedSets.insert(ptrb);
      }
    }
    
    std::map<int, int> previousCorners;
    
    #ifdef DEBUG
    std::cout << " LeftBeg " << leftBeginning << std::endl;
    #endif   
    
    int previous_found_corner = corners[leftBeginning - 1];
    
    for (int left = leftBeginning; left < this->size - 1; left++)
    {
      if (columnQEnds[left] == NOT_PRESENT) break; // || corners[left] == NOT_PRESENT?
      std::shared_ptr<const Element> leftPtr = ElementCreator::getInstance().getElementPtr(left + 1, this->size + 1);
      bool gotToEnd = true;
      for (int right = corners[left]; right <= columnQEnds[left]; right++)
      {
	if (right == previous_found_corner) 
	{
	  gotToEnd = false;
	  previousCorners.insert(std::make_pair(left, corners[left]));
	  corners[left] = right;
	  break; // idx 0 should always be NOT_PRESENT!
	}
	std::shared_ptr<const Element> rightPtr = ElementCreator::getInstance().getElementPtr(right + 1, this->size + 1);
	TableElement te(leftPtr, rightPtr);
	
	#ifdef DEBUG
	std::cout << te << std::endl;
	#endif
	
	std::unordered_map< TableElement, std::set< TableElement >* >::iterator kit = telToSet.find(te);
	// TODO - ale tohle nicmene MUZE BYT telToSet.end()
	std::set<TableElement> *assocLevel = (*kit).second;
	
	#ifdef DEBUG
	std::cout << "We think it is in assocLevel " << assocLevel << std::endl;
	#endif
	
	if (prohibitedSets.find(assocLevel) != prohibitedSets.end()) {
	  #ifdef DEBUG
	  std::cout << "Continue" << std::endl;
	  #endif
	  continue;
	}
	#ifdef DEBUG
	std::cout << "not prohibited" << std::endl;
	#endif
	if (nextLockSets.find(assocLevel) == nextLockSets.end() ) {
	  nextCornerSets.insert(assocLevel);
	  #ifdef DEBUG
	  std::cout << "not locked - add to nextCorners" << std::endl;
	  #endif
	}
	previousCorners.insert(std::make_pair(left, corners[left]));
	previous_found_corner = right;
	corners[left] = right;
	gotToEnd = false;
	#ifdef DEBUG
	std::cout << "Break" << std::endl;
        #endif
	break;
      }
      if (gotToEnd) {
	previousCorners.insert(std::make_pair(left, corners[left]));
	corners[left] = NOT_PRESENT;
	leftBeginning = left + 1;
      }
      
    }
    
#ifdef DEBUG    
    std::cout << "Corners" << std::endl;
    for (int i = 0; i < this->size - 1; i++)
    {
      std::cout << i << ": " << corners[i] << std::endl;
    }
    
    std::cout << "nextCorner" << std::endl;
    for (std::set<std::set<TableElement>*>::iterator c = nextCornerSets.begin(); c != nextCornerSets.end(); c++)
      {
	std::cout << *c << ": ";
	std::set<TableElement> *asdf = *c;
	for (std::set<TableElement>::iterator aa = asdf->begin(); aa != asdf->end(); ++aa)
	{
	  std::cout << *aa;
	}
      }
      std::cout << std::endl << std::endl << "prohibited" << std::endl;
    for (std::set<std::set<TableElement>*>::iterator c = prohibitedSets.begin(); c != prohibitedSets.end(); c++)
      {
	std::cout << *c << ": ";
	std::set<TableElement> *asdf = *c;
	for (std::set<TableElement>::iterator aa = asdf->begin(); aa != asdf->end(); ++aa)
	{
	  std::cout << *aa;
	}
      }
      
      std::cout << std::endl << std::endl << "locked" << std::endl;
      for (std::set<std::set<TableElement>*>::iterator c = nextLockSets.begin(); c != nextLockSets.end(); c++)
      {
	std::cout << *c << ": ";
	std::set<TableElement> *asdf = *c;
	for (std::set<TableElement>::iterator aa = asdf->begin(); aa != asdf->end(); ++aa)
	{
	  std::cout << *aa;
	}
      }
      std::cout << std::endl;
#endif
    // zavolat rekurzi
    
    checkVals(precededSets, res, primary, telToSet, nextCornerSets, nextLockSets);
    
    // vratit pole do stavu pred rekurzi
    for (std::map<int,int>::iterator pit = previousCorners.begin(); pit != previousCorners.end(); ++pit)
    {
      int pos = (*pit).first;
      int val = (*pit).second;
      corners[pos] = val;
    }
    
    leftBeginning = thisLevelStart;
  }
  
}

void printSets(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets,
				    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets,
				    std::set<std::set<TableElement>*>& ptrset)
{
  std::cerr << "Printing all sets:" << std::endl;
  for (auto it = ptrset.begin(); it != ptrset.end(); ++it)
  {
    std::set<TableElement> *set_ptr = *it;
    std::cerr << "Pointer set " << set_ptr << std::endl;
    for (auto it2 = set_ptr->begin(); it2 != set_ptr->end(); ++it2)
    {
      std::cerr << *it2 << std::endl;
    }
  }
  
  std::cerr << "Preceded sets:" << std::endl;
  for (auto it = precededSets.begin(); it != precededSets.end(); ++it)
  {
    std::set<TableElement> *key_ptr = (*it).first;
    auto *other_ptr = (*it).second;
    std::cerr << "For set " << key_ptr << std::endl;
    for (auto it2 = other_ptr->begin(); it2 != other_ptr->end(); ++it2)
    {
      std::cerr << *it2 << std::endl;
    }
  }
  std::cerr << std::endl;
  
  std::cerr << "Revert sets:" << std::endl;
  for (auto it = revertSets.begin(); it != revertSets.end(); ++it)
  {
    std::set<TableElement> *key_ptr = (*it).first;
    auto *other_ptr = (*it).second;
    std::cerr << "For set " << key_ptr << std::endl;
    for (auto it2 = other_ptr->begin(); it2 != other_ptr->end(); ++it2)
    {
      std::cerr << *it2 << std::endl;
    }
  }
  std::cerr << std::endl;
}

void recursiveTomonoidation(std::set<std::set<TableElement>*> *curr_preceded, 
			    std::map<TableElement, std::shared_ptr<const Element>> *rm,
			    std::unordered_set<std::set<TableElement>*> &assignedSet,
			    std::shared_ptr<const Element> atom,
			    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &precededSets,
			    std::set<std::set<TableElement>*>& nextZer,
			    std::unordered_map<std::set<TableElement>*, unsigned int> &sizes_map,
			    bool getLower
)
{
  auto end = assignedSet.end();
  for (auto it = curr_preceded->begin(); it != curr_preceded->end(); ++it)
  {
    std::set<TableElement> *eq_class = *it;
    if (getLower)
    {
      --sizes_map[eq_class];
      if (sizes_map[eq_class] == 0)
      {
	nextZer.insert(eq_class);
      }
    }
    if (assignedSet.find(eq_class) == end)
    {
      assignedSet.insert(eq_class);
      for (auto it2 = eq_class->begin(); it2 != eq_class->end(); ++it2)
      {
	rm->insert(std::make_pair(*it2, atom));
      }
      auto it2 = precededSets.find(eq_class);
      std::set< std::set< TableElement >* > *gosh = (*it2).second;
      recursiveTomonoidation(gosh, rm, assignedSet, atom, precededSets, nextZer, sizes_map, false);
    }
  }
}

void recount(std::set<TableElement> *current, 
	     std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &precededSets,
	     std::unordered_map<std::set<TableElement>*, unsigned int> &sizes_map,
	     std::stack<std::set<TableElement>*> &zeros,
	     std::unordered_set<std::set<TableElement>*> *subtractedSets
)
{
  auto sets_it = precededSets.find(current);
  std::set<std::set<TableElement>*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      if (subtractedSets->find(worset) == subtractedSets->end() )
      {
	int a = --sizes_map[worset];
	if (a == 0)
	{
	  zeros.push(worset);
	}
	subtractedSets->insert(worset);
      }
      recount(worset, precededSets, sizes_map, zeros, subtractedSets);
    }
}

void assignAtoms(std::set<TableElement> *current, 
		std::map<TableElement, std::shared_ptr<const Element>> *rm,
		std::shared_ptr<const Element> atom,
		std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &precededSets,
		std::unordered_set<std::set<TableElement>*> &assignedSets,
		std::unordered_set<std::set<TableElement>*> *newlyAssignedSets
)
{
  // assign 1s in *current set 
  for (auto it = current->begin(); it != current->end(); ++it)
  {
    rm->insert(std::make_pair(*it, atom));
  }
  auto sets_it = precededSets.find(current);
  
  std::set<std::set<TableElement>*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
  {
    std::set<TableElement> *nextSet = *it;
    if (assignedSets.find(nextSet) == assignedSets.end() )
    {
      assignedSets.insert(nextSet);
      newlyAssignedSets->insert(nextSet);
      assignAtoms(nextSet, rm, atom, precededSets, assignedSets, newlyAssignedSets);
    }
  }
    
}

void Tomonoid::assignOthersRecursive(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &revertSets,
		  std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &precededSets,
		  std::unordered_set<std::set<TableElement>*> &assignedSet,
		  std::vector<Tomonoid*> &res,
		  Tomonoid *primary,
		  std::unordered_map<std::set<TableElement>*, unsigned int> &sizes_map,
		  std::stack<std::set<TableElement>*> &zeros,
		  std::shared_ptr<const Element> atom
		)
{
  if (!zeros.empty())
  {
    std::set<TableElement> *current = zeros.top();
    zeros.pop();
    std::stack<std::set<TableElement>*> original_zeros = std::stack<std::set<TableElement>*>(zeros);
    Tomonoid *tnew = new Tomonoid(*primary);
    results_map *rm = new results_map(primary->importantResults);
    std::unordered_set<std::set<TableElement>*> *newlyAssignedSets = new std::unordered_set<std::set<TableElement>*>();
    // force recursively all preceded sets to atom
    assignedSet.insert(current);
    assignAtoms(current, rm, atom, precededSets, assignedSet, newlyAssignedSets);
    tnew->setImportantResults(*rm);
    delete rm;
    assignOthersRecursive(revertSets, precededSets, assignedSet, res, tnew, sizes_map, zeros, atom);
    res.push_back(tnew);
    // and now act like this is set to 0
    for (auto it = newlyAssignedSets->begin(); it != newlyAssignedSets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      assignedSet.erase(worset);
    }
    newlyAssignedSets->clear();
    recount(current, precededSets, sizes_map, original_zeros, newlyAssignedSets);
    delete newlyAssignedSets;
    assignOthersRecursive(revertSets, precededSets, assignedSet, res, primary, sizes_map, original_zeros, atom);
  }
  /*
  std::set<std::set<TableElement>*> next_zer(zeros);
  std::unordered_set<std::set<TableElement>*> orig_assig(assignedSet);
  for (auto zit = zeros.begin(); zit != zeros.end(); ++zit)
  {
    std::set<TableElement> *current = *zit;
    next_zer.erase(current);
    Tomonoid *tnew = new Tomonoid(*primary);
    results_map *rm = new results_map(primary->importantResults);
    // assign 1s in *current set 
    for (auto it = current->begin(); it != current->end(); ++it)
    {
	rm->insert(std::make_pair(*it, atom));
    }
    // enforce 1s in preceded sets
    auto ity = *(precededSets.find(current));
    std::set<std::set<TableElement>*> *curr_preceded = ity.second;
    assignedSet.insert(current);
    recursiveTomonoidation(curr_preceded, rm, assignedSet, atom, precededSets, next_zer, sizes_map, true);
    tnew->setImportantResults(*rm);
    delete rm;
    // recursively assign others
    assignOthersRecursive(revertSets, precededSets, assignedSet, res, tnew, sizes_map, next_zer, atom);
    // now act like this is 0 forever and recursively assign others
    res.push_back(tnew);
    assignedSet = orig_assig;
    /*assignOthersRecursive(revertSets, precededSets, assignedSet, res, primary, sizes_map, next_zer, atom);
    for (auto it = curr_preceded->begin(); it != curr_preceded->end(); ++it)
    {
      std::set<TableElement> *eq_class = *it;
      ++sizes_map[eq_class];
    }
  }*/
}

void Tomonoid::assignOthers(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets,
		  std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets,
		  std::set<std::set<TableElement>*>& ptrset,
		  std::vector<Tomonoid*> &res,
		  Tomonoid *primary,
		  std::unordered_map< TableElement, std::set< TableElement >* >& telToSet)
{
    // od tech, co maji "aktualni" revert nulovy (tj. jsou na zacatku grafu)
  // volani rekurzivne, vzdycky odebirat vynucene sety! + pridat jeste nejaky set pracovnich pointeru
  // vybrane mnozine se priradi jednicka
  std::unordered_map<std::set<TableElement>*, unsigned int> sizes_map;
  std::stack<std::set<TableElement>*> starting_zeros;
  for (auto it = precededSets.begin(); it != precededSets.end(); ++it)
  {
    std::set<TableElement> *el_class = (*it).first;
    auto set_iterator = revertSets.find(el_class);
    unsigned int cnt = 0;
    if (set_iterator != revertSets.end() )
    {
      std::set< std::set< TableElement >* > *sets = (*set_iterator).second;
      cnt = sets->size();
    }
    else
    {
	starting_zeros.push(el_class);
    }
    sizes_map.insert(std::make_pair(el_class, cnt));
    #ifdef DEBUG
    std::cerr << "For set " << el_class << " there are " << cnt << " entries in revert." << std::endl;
    #endif
  }
  std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
  std::unordered_set<std::set<TableElement>*> takenSet;
  assignOthersRecursive(revertSets, precededSets, takenSet, res, primary, sizes_map, starting_zeros, atom);
}

void Tomonoid::assignThroughCorners(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets,
				    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets,
				    std::set<std::set<TableElement>*>& ptrset,
			    std::vector<Tomonoid*> &res,
			    Tomonoid *primary,
			    std::unordered_map< TableElement, std::set< TableElement >* >& telToSet)
{
  std::set<std::set<TableElement>*> cornerSets;
  
  #ifdef DEBUG
  TomonoidPrinter tp;
  tp.printTomonoid(this);
  
  printSets(revertSets, precededSets, ptrset);
  #endif
  
  this->corners = new int[this->size - 1];
  
  for (int column = 0; column < this->size - 1; column++)
  {
    corners[column] = NOT_PRESENT;
    #ifdef DEBUG
    std::cout << " Check column " << column << std::endl;
    #endif
    
    if (columnQEnds[column] == NOT_PRESENT || zeroCol[column] == NOT_PRESENT) continue;
    
    std::shared_ptr<const Element> leftEl = ElementCreator::getInstance().getElementPtr(column + 1, this->size + 1);
    for (int right = zeroCol[column] + 1; right <= columnQEnds[column]; right++)
    {
      #ifdef DEBUG
      std::cout << " Check row " << right << std::endl;
      #endif
      /*if (column != 0 && right == corners[column - 1 ])
      {
	corners[column] = right;
	break;
      }*/
      std::shared_ptr<const Element> rightEl = ElementCreator::getInstance().getElementPtr(right + 1, this->size + 1);
      TableElement te(leftEl, rightEl);
      
      std::unordered_map< TableElement, std::set< TableElement >* >::iterator it = telToSet.find(te);
      if (it == telToSet.end() )
      {
	#ifdef DEBUG
	std::cout << "Not in telToSet" << std::endl;
	#endif
	continue;
      }
      
      std::set< TableElement > *assocSet = (*it).second;
      
      std::set<std::set<TableElement>*>::iterator it2;
      it2 = ptrset.find(assocSet);
      
      if (it2 == ptrset.end() )
      {
	#ifdef DEBUG
	std::cout << "Not in ptrSet" << std::endl;
	#endif
	continue;
      }
      
      if (revertSets.size() == 0)
      {
	corners[column] = right;
	cornerSets.insert(assocSet);
	break;
      }
      
      std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator it3;
      it3 = revertSets.find(assocSet);
      
      if (it3 == revertSets.end() )
      {
	corners[column] = right;
	cornerSets.insert(assocSet);
	break;
      }
      
      std::set< std::set< TableElement >* > *revertsSets = (*it3).second; 
      
      corners[column] = right;
      
      if (revertsSets->empty()) // otherwise something precedes it and we need to leave it for next iterations
      {
	cornerSets.insert(assocSet);
      }
      
      break;
    }
  }
  
  for (int i = 0; i < this->size - 1; i++)
  {
    if (corners[i] != NOT_PRESENT) 
    {
      leftBeginning = i;
      break;
    }
  }
  
  #ifdef DEBUG
  std::cout << "Corners" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cout << i << ": " << corners[i] << std::endl;
  }
  
  std::cout << "nextCorner" << std::endl;
    for (std::set<std::set<TableElement>*>::iterator c = cornerSets.begin(); c != cornerSets.end(); c++)
      {
	std::cout << *c << ": ";
	std::set<TableElement> *asdf = *c;
	for (std::set<TableElement>::iterator aa = asdf->begin(); aa != asdf->end(); ++aa)
	{
	  std::cout << *aa << std::endl;
	}
      }
  #endif
  
  std::set<std::set<TableElement>*> lockSets;
  checkVals(precededSets, res, primary, telToSet, cornerSets, lockSets);

  delete[] this->corners;
}

void Tomonoid::doAssignmentLoop(std::set<std::set<TableElement>*>& set,
		      results_map &newmap,
		      std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets
)
{
  std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
  for (std::set< std::set< TableElement >* >::iterator it = set.begin(); it != set.end(); ++it)
  {
    std::set< TableElement > *assocs = *it;
    
    for (std::set< TableElement >::iterator fin = assocs->begin(); fin != assocs->end(); ++fin)
    {
      TableElement te = *fin;
      newmap.insert(std::make_pair(te, atom));
    }
    
    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator val;
    val = precededSets.find(assocs);
    if (val != precededSets.end() )
    {
      std::set< std::set< TableElement >* > *precs = (*val).second;
      for (std::set< std::set< TableElement >* >::iterator precIt = precs->begin(); precIt != precs->end(); ++precIt)
      {
	std::set< TableElement > *oneSet = *precIt;
	for (std::set< TableElement >::iterator fin = oneSet->begin(); fin != oneSet->end(); ++fin)
	{
	  TableElement te = *fin;
	  newmap.insert(std::make_pair(te, atom));
	}
      }
    }
  }
}

// only assignment of values
void Tomonoid::assignRecursively(results_map &newmap,
		       std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets,
		       std::set< std::set< TableElement >* >& current,
		       std::set< std::set< TableElement >* >& lock
				)
{
  doAssignmentLoop(lock, newmap, precededSets);
  doAssignmentLoop(current, newmap, precededSets);
}

void Tomonoid::goThroughGraph(std::set< std::set< TableElement >* >& current, 
		      std::unordered_map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& next,
		      std::set< std::set< TableElement >*>& locks,
		      Tomonoid* primary,
		      std::vector<Tomonoid*> &res)
{
  // 1st trivial - pick all
  
  // 2nd - pick all combinations and recursively assign them...
}

void Tomonoid::assignThroughGraph(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
			      std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets,
			      std::set<std::set<TableElement>*>& ptrset,
			      std::vector<Tomonoid*> &res,
			      Tomonoid *primary
 				)
{
  // TODO
  // first check cycles
  
  // merge cycled values
  
  std::unordered_map<std::set<TableElement>*, std::set<std::set<TableElement>*>*> next;
  
  std::set<std::set<TableElement>*> free;
  std::set<std::set<TableElement>*> locks;
  
  for (std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator it = revertSets.begin();
       it != revertSets.end(); ++it)
       {
	 std::set< std::set< TableElement >* > *revs = (*it).second;
	 if (revs->empty() )
	 {
	    std::set< TableElement > *currset = (*it).first;
	    std::set<std::set<TableElement>*> *nexts = new std::set<std::set<TableElement>*>();
	    next.insert(std::make_pair(currset, nexts));
	    free.insert(currset);
	    revertSets.erase(currset);
	    delete revs;
	}
      }
  
  for (std::unordered_map<std::set<TableElement>*, std::set<std::set<TableElement>*>*>::iterator it = next.begin();
       it != next.end(); ++it)
       {
	 std::set<TableElement> *del = (*it).first;
	 std::cout << "In first level There is " << del << std::endl;
      }
  
  
  
  /*
  int curr_level = 0;
  while(revertSets.size() > 0)
  {
    std::set<std::set<TableElement>*> next;
    for (std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator it = revertSets.begin();
	it != revertSets.end(); ++it)
    {
	  std::set<TableElement> *assocs = (*it).first;
	  std::set< std::set< TableElement >* > *reverts = (*it).second;
	  if (reverts.size() == 0)
	  {
	    next.insert(assocs);
	  }
    }
    for (std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator it = revertSets.begin();
	it != revertSets.end(); ++it)
    {
	  
    }
    curr_level++;
  }*/
  
  //goThroughGraph(free, next, locks, primary, res);
  
  for (std::unordered_map<std::set<TableElement>*, std::set<std::set<TableElement>*>*>::iterator it = next.begin();
       it != next.end(); ++it)
       {
	 std::set<std::set<TableElement>*> *del = (*it).second;
	 delete del;
      }
  
}