#include "tomonoid.h"

Tomonoid::GraphAssignator::GraphAssignator(mapset_type *precs, 
		    mapset_type *revs, 
		    Tomonoid *prims,
		    std::vector<Tomonoid*> *results
 		  )
: precededSets(precs), revertSets(revs), primary(prims), res(results) 
{
	atom = ElementCreator::getInstance().getElementPtr(1, prims->size);
};

void Tomonoid::GraphAssignator::doAssignment()
{
      // od tech, co maji "aktualni" revert nulovy (tj. jsou na zacatku grafu)
  // volani rekurzivne, vzdycky odebirat vynucene sety! + pridat jeste nejaky set pracovnich pointeru
  // vybrane mnozine se priradi jednicka
  std::stack<associated_set*> *zeros = new std::stack<associated_set*>();
  for (auto it = precededSets->begin(); it != precededSets->end(); ++it)
  {
    std::set<TableElement> *el_class = (*it).first;
    auto set_iterator = revertSets->find(el_class);
    unsigned int cnt = 0;
    if (set_iterator != revertSets->end() )
    {
      std::set< std::set< TableElement >* > *sets = (*set_iterator).second;
      cnt = sets->size();
    }
    if (cnt == 0)
    {
      zeros->push(el_class);
    }
    sizes_map.insert(std::make_pair(el_class, cnt));
    #ifdef DEBUG
    std::cerr << "For set " << el_class << " there are " << cnt << " entries in revert." << std::endl;
    #endif
  }
  assignRecursively(zeros, primary);
  delete zeros;
}

void Tomonoid::GraphAssignator::assignRecursively(std::stack<associated_set*> *zeros, Tomonoid *nowtom)
{
  if (!zeros->empty())
  {
    associated_set *current = zeros->top();
    zeros->pop();
    std::stack< associated_set* > *original_zeros = new std::stack< associated_set* >(*zeros);
    Tomonoid *tnew = new Tomonoid(*nowtom);
    results_map *rm = new results_map(nowtom->importantResults);
    assigned_sets *newlyAssignedSets = new assigned_sets();
    // force recursively all preceded sets to atom
    takenSet.insert(current);
    assignAtoms(current, rm, newlyAssignedSets);
    tnew->setImportantResults(*rm);
    
#ifdef VERBOSE
    std::cout << "Top: " << current << std::endl;
    TomonoidPrinter tp;
    tp.printTomonoid(tnew);
    std::cout << "1 - go to next" << std::endl;
#endif
    
    delete rm;
    assignRecursively(zeros, tnew);
    delete tnew;
    // and now act like this is set to 0
    for (auto it = newlyAssignedSets->begin(); it != newlyAssignedSets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      takenSet.erase(worset);
    }
    delete newlyAssignedSets;
    recount(current, original_zeros);
#ifdef VERBOSE
    std::cout << "Now return to " << current << std::endl;
    std::cout << "Set it 0" << std::endl;
#endif
    assignRecursively(original_zeros, nowtom);
    delete original_zeros;
    
    recountBack(current);
    takenSet.erase(current);
  }
  else
  {
#ifdef VERBOSE
    std::cout << "Saving" << std::endl;
    TomonoidPrinter tp;
    tp.printTomonoid(nowtom);
#endif
    Tomonoid *tnew = new Tomonoid(*nowtom);
    results_map rm(nowtom->importantResults);
    tnew->setImportantResults(rm);
    res->push_back(tnew);
  }
}

void Tomonoid::GraphAssignator::assignAtoms(associated_set* current, results_map *rm, assigned_sets *newlyAssignedSets)
{
  // assign 1s in *current set 
  for (auto it = current->begin(); it != current->end(); ++it)
  {
    rm->insert(std::make_pair(*it, atom));
  }
  auto sets_it = precededSets->find(current);
  
  std::set<std::set<TableElement>*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
  {
    std::set<TableElement> *nextSet = *it;
    if (takenSet.find(nextSet) == takenSet.end() )
    {
      takenSet.insert(nextSet);
      newlyAssignedSets->insert(nextSet);
      assignAtoms(nextSet, rm, newlyAssignedSets);
    }
  }
}

void Tomonoid::GraphAssignator::recountBack(associated_set *current)
{
  auto sets_it = precededSets->find(current);
  std::set<associated_set*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
    {
      associated_set *worset = *it;
      int a = ++sizes_map[worset];
    }
}

void Tomonoid::GraphAssignator::recount(associated_set *current, std::stack<associated_set*> *zeros)
{
  auto sets_it = precededSets->find(current);
  std::set<std::set<TableElement>*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      int a = --sizes_map[worset];
      if (a == 0)
      {
	zeros->push(worset);
      }
    }
}


void recountBack(std::set<TableElement> *current, 
	     std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &precededSets,
	     std::unordered_map<std::set<TableElement>*, unsigned int> &sizes_map)
{
  auto sets_it = precededSets.find(current);
  std::set<std::set<TableElement>*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      int a = ++sizes_map[worset];
    }
}

void recount(std::set<TableElement> *current, 
	     std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* > &precededSets,
	     std::unordered_map<std::set<TableElement>*, unsigned int> &sizes_map,
	     std::stack<std::set<TableElement>*> &zeros//,
	     //std::unordered_set<std::set<TableElement>*> *subtractedSets
)
{
  auto sets_it = precededSets.find(current);
  std::set<std::set<TableElement>*> *sets = (*sets_it).second;
  for (auto it = sets->begin(); it != sets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      //if (subtractedSets->find(worset) == subtractedSets->end() )
      //{
	int a = --sizes_map[worset];
	if (a == 0)
	{
	  zeros.push(worset);
	}
	//subtractedSets->insert(worset);
      //}
      //recount(worset, precededSets, sizes_map, zeros, subtractedSets);
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
    
#ifdef DEBUG
    std::cout << "Top: " << current << std::endl;
    TomonoidPrinter tp;
    tp.printTomonoid(tnew);
    std::cout << "1 - go to next" << std::endl;
#endif
    
    delete rm;
    assignOthersRecursive(revertSets, precededSets, assignedSet, res, tnew, sizes_map, zeros, atom);
    delete tnew;
    // and now act like this is set to 0
    for (auto it = newlyAssignedSets->begin(); it != newlyAssignedSets->end(); ++it)
    {
      std::set<TableElement> *worset = *it;
      assignedSet.erase(worset);
    }
    delete newlyAssignedSets;
    //newlyAssignedSets->clear();
    //newlyAssignedSets->insert(assignedSet.begin(), assignedSet.end());
    recount(current, precededSets, sizes_map, original_zeros);//, newlyAssignedSets);
#ifdef DEBUG
    std::cout << "Now return to " << current << std::endl;
    std::cout << "Set it 0" << std::endl;
#endif
    assignOthersRecursive(revertSets, precededSets, assignedSet, res, primary, sizes_map, original_zeros, atom);
    
    recountBack(current, precededSets, sizes_map);
    assignedSet.erase(current);
  }
  else
  {
#ifdef DEBUG
    std::cout << "Saving" << std::endl;
    TomonoidPrinter tp;
    tp.printTomonoid(primary);
#endif
    Tomonoid *tnew = new Tomonoid(*primary);
    results_map rm = results_map(primary->importantResults);
    tnew->setImportantResults(rm);
    res.push_back(tnew);
  }
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
    if (cnt == 0)
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
