#include "tomonoid.h"
#define LOOPING_THRESHOLD 6 // No. of associated sets to which looping over all combinations will be used 

#ifdef CONTROL
#include <mutex>
std::mutex control_print_mutex;
#endif

inline unsigned int Tomonoid::posInHelpers(unsigned int order)
{
  return this->size - 1 - order;
}

void printAllInfo(std::set<std::set<TableElement>*>& ptrset,
		  std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
		  std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets);

void Tomonoid::calExtFromIdempots(std::vector<Tomonoid*>& res, 
				  const Element& el, 
				  const Element& er, 
				  associated_mapset associatedValues )
{

#ifdef VERBOSE
  std::cerr << "Now calculating for idempotent pair " << std::endl;
  std::cerr << "Left: " << el << std::endl;
  std::cerr << "Right: " << er << std::endl;
#endif
  
  // step 4 - omitted (results are in this->importantResults or in previous levels)
  
  // step 5
  // 1 in P but can be omitted as it provides no additional information about associativity
  // assoc: for every (a,b), (c,d) in P let: (a,b) = d, (b,c) = e; then (a,e) = (d,c)
  for (int i = 0; i < this->size - 1; i++)
  {
    atomCol[i] = UNASSIGNED;
    atomRow[i] = UNASSIGNED;
    zeroCol[i] = UNASSIGNED;
    zeroRow[i] = UNASSIGNED;
  }
  
  //std::cout << "Step 4" << std::endl;
  stepE4(el, er);
  //std::cout << "Step 3a" << std::endl;
  stepE3a(el, er);
  //std::cout << "Step 3c" << std::endl;
  stepE3c(el, er);
  //std::cout << "Step 3b" << std::endl;
  stepE3b(el, er, associatedValues); 
  
  // check and repair helper arrays so they may be used in next steps
  editZerosAndAtoms();
  // to be sure, add values in Q that will not be forcibly set to 0 or atom
  controlFreeValues(associatedValues);
  
  // merge all associated sets
  std::unordered_multimap<std::set<TableElement> *, TableElement> setToTel;
  std::unordered_map<TableElement, std::set<TableElement> * > telToSet;
  
  mergeAssociatedValues(associatedValues, setToTel, telToSet);
  
#ifdef VERBOSE
  std::cerr << "Go print" << std::endl;
  
  std::cerr << "Printing zero ends" << std::endl;
  
  std::cerr << "Zerocol" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cerr << i << ": " <<  zeroCol[i] << std::endl; 
  }
  std::cerr << "zeroRow" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cerr << i << ": " <<  zeroRow[i] << std::endl; 
  }
  std::cerr << "atomCol" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cerr << i << ": " <<  atomCol[i] << std::endl; 
  }
  std::cerr << "atomRow" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cerr << i << ": " <<  atomRow[i] << std::endl; 
  }
  std::cerr << "QCol" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cerr << i << ": " <<  columnQEnds[i] << std::endl; 
  }
  std::cerr << "QRow" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cerr << i << ": " <<  rowQEnds[i] << std::endl; 
  }
#endif

  std::set<std::set<TableElement>*> ptrset;
  
  int iter = 0;
  for (auto ajty = setToTel.begin(); ajty != setToTel.end(); ++ajty)
  {
    std::set<TableElement> *setTel = (*ajty).first;    
    ptrset.insert(setTel);
    
  }
  
  // zero-filled (+ previous values) new tomonoid
  // as if all free results were set to 0
  // base tomonoid for final assignment process and first result too
  Tomonoid *nextTomo = new Tomonoid(this);
  
#ifdef VERBOSE
  int dd = 0;
  std::cerr << "b4 assignments " << std::endl;
  for (std::set<std::set<TableElement>*>::iterator sit = ptrset.begin(); sit != ptrset.end(); ++sit)
  {
    std::cerr << "Iteration " << dd << ", address: " << *sit << std::endl;
    for (std::set<TableElement>::iterator eas = (*sit)->begin(); eas != (*sit)->end(); ++eas)
    {
      std::cerr << *eas << std::endl;
    }
    dd++;
  }
#endif
  
  // assign atoms to associated sets that have to result in atom only (and remove them)
  bool atomsOk = assignAtom(ptrset, telToSet, nextTomo);
  
  // actually delete those remaining associated sets, that must be zero, so in final we'll have 
  // only sets that might be assigned both atom or zero
  
  std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* > precededSets;
  std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* > revertSets;
  calcPrecedings(precededSets, revertSets, ptrset);
  
#ifdef VERBOSE
 int kk = 0;
  std::cout << "after calcPrec rev" << std::endl;
  for (std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >*>::iterator sit = revertSets.begin(); 
       sit != revertSets.end(); ++sit)
  {
    std::cout << "Iteration " << kk << ", address: " << (*sit).first << std::endl;
    std::set<std::set<TableElement>*>* p2 = (*sit).second;
    for (std::set<std::set<TableElement>*>::iterator eas = p2->begin(); eas != p2->end(); ++eas)
    {
      std::cout << "rev_adres: " << *eas << std::endl;
    }
    kk++;
  }
#endif
  
#ifdef VERBOSE
  std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* >::iterator iiit = revertSets.begin();
    for (iiit; iiit != revertSets.end(); ++iiit)
    {
      std::set<TableElement>* key = (*iiit).first;
      std::set<std::set<TableElement>*>* setDusets = (*iiit).second;
      
      std::cout << "For associated set " << key << " there are these reverted:" << std::endl;
      
      for (std::set<std::set<TableElement>*>::iterator it = setDusets->begin(); it != setDusets->end(); ++it)
      {
	std::cout << "Connected sets, iteration " << iter << std::endl;
	std::set<TableElement> *setTel = (*it);
	std::cout << "Address: " << setTel << std::endl;
	std::set<TableElement>::iterator it2 = setTel->begin();
	for (it2; it2 != setTel->end(); ++it2)
	{
	    std::cout << *it2 << std::endl;
	}
	iter++;
      }
    }
#endif
  
  if (!atomsOk)
  {
    #ifdef DEBUG
    std::cerr << "CANNOT ACCEPT THIS EXTENSION!" << std::endl;
    #endif
    
    std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* >::iterator iiit = precededSets.begin();
    for (iiit; iiit != precededSets.end(); ++iiit)
    {
      std::set<TableElement>* key = (*iiit).first;
      std::set<std::set<TableElement>*>* setDusets = (*iiit).second;
      delete setDusets;
    }
    
    std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* >::iterator reviii = revertSets.begin();
    for (reviii; reviii != revertSets.end(); ++reviii)
    {
      std::set<std::set<TableElement>*>* setDusets = (*reviii).second;
      delete setDusets;
    }
  
    for (std::set<std::set<TableElement>*>::iterator it = ptrset.begin(); it != ptrset.end(); ++it)
    {
      std::set<TableElement> *setTel = (*it);
      delete setTel;
      iter++;
    }
    return; // This is invalid.
  }
  assignZeros(ptrset, precededSets, revertSets);
  
  
#ifdef VERBOSE
  std::cerr << "After assignZeros" << std::endl;
  printAllInfo(ptrset, precededSets, revertSets);
#endif

  
  if (el != Element::top_element || er != Element::top_element)
  {
    nArchAssocCheck();
  }
  
  // So there remains only those from which we can iterate :)
  // Original nextTomo -> assignation with all zeros
  validPermutations(res, ptrset, nextTomo, precededSets, revertSets);
}

void Tomonoid::nArchAssocCheck()
{
  
}

void Tomonoid::mergeAssociatedValues(associated_mapset& associatedValues, 
			     std::unordered_multimap< std::set< TableElement >*, TableElement >& setToTel, 
			     std::unordered_map< TableElement, std::set< TableElement >* >& telToSet)
{
  associated_mapset::iterator it = associatedValues.begin();
  //int i = 0;
  for (it; it != associatedValues.end(); ++it)
  {
    //std::cout << "Mg Iteration " << i << std::endl;
    std::set<TableElement> &set = (*it).second; // val from associated values -> all associated with key
    const TableElement &te = (*it).first; // Key from associated values
    
    std::set<TableElement> *teNewSet = new std::set<TableElement>();
    teNewSet->insert(te);
    telToSet.insert(std::pair< TableElement, std::set< TableElement >* >(te, teNewSet) );
    
    //std::cout << "Associated values for " << te << ":" << std::endl;
    //std::cout << te.getHash() << std::endl;
    
    std::set<TableElement>::iterator it2 = set.begin();
    std::set<std::set<TableElement>*> toDel;
    for (it2; it2 != set.end(); ++it2)
    {
      const TableElement& sette = *it2; // associated with te
      std::unordered_map< TableElement, std::set< TableElement >* >::iterator it3 = telToSet.find(sette);
      // those we already know
      if (it3 != telToSet.end())
      {
	//std::cout << "Mg if 1" << std::endl;
	if (teNewSet->count( (*it3).first ) == 0) // value missing and 
	{
	  //std::cout << "Mg if" << std::endl;
	  std::set<TableElement>* prev_ptr = (*it3).second;
	  //std::cout << "Referencing " << prev_ptr << std::endl;
	  teNewSet->insert(prev_ptr->begin(), prev_ptr->end());
	  for (std::set<TableElement>::iterator eet = prev_ptr->begin(); eet != prev_ptr->end(); ++eet)
	  {
	    telToSet.erase(*eet);
	    telToSet.insert(std::pair< TableElement, std::set< TableElement >* >(*eet, teNewSet));
	  }
	  
	  telToSet.erase(sette);
	  telToSet.insert(std::pair< TableElement, std::set< TableElement >* >(sette, teNewSet));
	  toDel.insert(prev_ptr);
	}
      }
      else
      {
	//std::cout << "Mg if else" << std::endl;
	teNewSet->insert(sette);
	telToSet.insert(std::pair< TableElement, std::set< TableElement >* >(sette, teNewSet));
      }
    }
    //std::cout << "About deleting" << std::endl;
    for (std::set<std::set<TableElement>*>::iterator ita = toDel.begin(); ita != toDel.end(); ++ita)
    {
      //std::cout << "Deleting " << *ita << std::endl;
      delete *ita;
    }
    //++i;
  }
  std::unordered_map< TableElement, std::set< TableElement >* >::iterator it4 = telToSet.begin();
  
  for (it4; it4 != telToSet.end(); ++it4)
  {
    const TableElement &nv = (*it4).first;
    std::set<TableElement> *setptr = (*it4).second;
    setToTel.insert(std::pair<std::set<TableElement>*, TableElement>(setptr, nv) );
  }
}

void Tomonoid::stepE4(const Element& el, const Element& er)
{
  // step 6 - implicitly
  //   step 7 - (a, atom) = 0 and (atom, b) = 0 for all a < el, b < er
  // step 8 - (a, atom) = atom and (atom, b) = atom for all a >= el, b >= er
  
  /*7. Perform (a, α) ∼ (α, b) ∼ 0 for a < ε l and b < ε r .
8. Perform (ε l , α) ∼ (α, ε r ) ∼ α.*/
  
  if (el == Element::top_element)
  {
    zeroRow[0] = this->size - 2;
    zeroCol[this->size - 2] = 0;
    atomRow[0] = NOT_PRESENT;
  }
  else
  {
    int order = el.getOrder();
    zeroRow[0] = this->size - 2 - order;
    zeroCol[this->size - 2 - order] = 0;
    zeroCol[this->size - 1 - order] = NOT_PRESENT;
    atomRow[0] = this->size - 1 - order;
    atomCol[this->size - 1 - order] = 0;
  }
  // first assignment - nothing to care about
  
  if (er == Element::top_element)
  {
    zeroCol[0] = this->size - 2; // nemuze byt NOT_PRESENT, maximalne prepisu nulu
    zeroRow[this->size - 2] = 0; // nikdy by nemela byt nula (krome trivialniho a tam to nevadi)
    atomCol[0] = NOT_PRESENT; // k tomu bychom se opet nemeli nikdy dostat 
  }
  else
  {
    int order = er.getOrder();
    zeroCol[0] = this->size - 2 - order; // nemuze byt NOT_PRESENT, maximalne prepisu nulu
    zeroRow[this->size - 2 - order] = std::max<int>(0, zeroRow[this->size - 2 - order]); // tady potencialne hrozi prepsani
    zeroRow[this->size - 1 - order] = NOT_PRESENT; // neni nikdy nula
    atomCol[0] = this->size - 1 - order; // z predchoziho se sem nedostanu
    atomRow[this->size - 1 - order] = 0; // neni nula
  }
}

void Tomonoid::stepE3c(const Element& el, const Element& er)
{
/*
 * 11. For every a ∈ S̄ such that ε l <= a < 1:
  – let b ∈ S̄ be the highest element such that (a, b) ∈ Q,
  – let c ∈ S̄ be the highest element such that (b, c) ∈ Q and c < ε r ,
  – perform (b, c) ∼
  ̇ 0.
 */
  int c_max = er == Element::top_element ? this->size - 2 : posInHelpers(er.getOrder() + 1);
  //std::cout << "c_max: " << c_max << std::endl;
  
  if (el != Element::top_element)
  {
    //std::cout << "Non-top el" << std::endl;
    for (int a = this->size - 1 - el.getOrder(); a < this->size - 1; a++)
    {
      int b = columnQEnds[a];
      int c = std::min(columnQEnds[b], c_max);
      //std::cout << "a = " << a << ", b = " << b << ", c = " << c << std::endl;
      if (zeroCol[b] < c)
      {
	zeroCol[b] = c;
      }
      if (zeroRow[c] < b)
      {
	zeroRow[c] = b;
      }
    }
  }
  
  /*
   12. For every c ∈ S̄ such that ε r <= c < 1:
  – let b ∈ S̄ be the highest element such that (b, c) ∈ Q,
  – let a ∈ S̄ be the highest element such that (a, b) ∈ Q and a < ε l ,
  – perform (a, b) ∼
  ̇ 0.*/
  int a_max = el == Element::top_element ? this->size - 2 : posInHelpers(el.getOrder() + 1); // o jednu mensi -> o jednu vetsi order
  //std::cout << "a_max: " << a_max << std::endl;
  
  if (er != Element::top_element)
  {
    for (int c = this->size - 1 - er.getOrder(); c < this->size - 1; c++)
    {
      //std::cout << "Non-top er" << std::endl;
      int b = rowQEnds[c];
      int a = std::min(rowQEnds[b], a_max);
      //std::cout << "c = " << c << ", b = " << b << ", a = " << a << std::endl;
      if (zeroRow[b] < a)
      {
	zeroRow[b] = a;
      }
      if (zeroCol[a] < b)
      {
	zeroCol[a] = b;
      }
    }
  }
  
}


void Tomonoid::stepE3a(const Element& el, const Element& er)
{
  /*For every a ∈ S̄ such that α < a < ε l :
  – let b ∈ S̄ be the highest element such that (a, b) ∈ Q,
  – let c ∈ S̄ be the highest element such that c < ε r ,
  – let e ∈ S̄ be such that (b, c) ∼ e,
  – if e > α then perform (a, e) ∼
  ̇ 0.*/
  std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
  const Element& atomRef = *(atom.get());
  
  // "el - 1"
  std::shared_ptr<const Element> c1;
  if (er == Element::top_element)
  {
    c1 = ElementCreator::getInstance().getElementPtr(this->size - 1, this->size + 1);
  }
  else
  {
    c1 = ElementCreator::getInstance().getElementPtr(this->size - 1 - er.getOrder(), this->size + 1);;
  }
  
  if (atomRef != *( c1.get() ) ) // otherwise the result will always be atom or 0
  {
    for (int a = 1; a < this->size - 1 - el.getOrder(); a++)
    {
      int b = columnQEnds[a];
      if (b == this->size - 1) 
      {
	continue;
      }
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b + 1, this->size + 1);
      if (*( bPtr.get() ) != atomRef)
      {
	const Element& e_res = this->getResult(bPtr, c1);
	if (e_res > atomRef)
	{
	  int pos = posInHelpers(e_res.getOrder());
	  zeroCol[a] = std::max<int>(pos, zeroCol[a]); // always first assigning, only one for each a in loop
	  zeroRow[pos] = std::max<int>(a, zeroRow[pos]); // if not first assigning, then it is greater than previous anyway
	}
      }
    }
  }
  
  
  /*For every c ∈ S̄ such that α < c < ε r :
  – let b ∈ S̄ be the highest element such that (b, c) ∈ Q,
  – let a ∈ S̄ be the highest element such that a < ε l ,
  – let d ∈ S̄ be such that (a, b) ∼ d,
  – if d > α then perform (d, c) ∼
 ̇ 0.
   */
  
  std::shared_ptr<const Element> a2;
  if (el == Element::top_element)
  {
    a2 = ElementCreator::getInstance().getElementPtr(this->size - 1, this->size + 1);
  }
  else
  {
    a2 = ElementCreator::getInstance().getElementPtr(this->size - 1 - el.getOrder(), this->size + 1);;
  }
  
  if (*( atom.get() ) != *( a2.get() ) ) // otherwise the result will always be atom or 0
  {
    for (int c = 1; c < this->size - 1 - er.getOrder(); c++)
    {
      int b = rowQEnds[c];
      if (b == this->size - 1) 
      {
	continue;
      }
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b + 1, this->size + 1);
      if (*( bPtr.get() ) != atomRef)
      {
	const Element& d_res = this->getResult(a2, bPtr);
	if (d_res > atomRef)
	{
	  int pos = posInHelpers(d_res.getOrder());
	  if (zeroRow[c] < pos) // don't overwrite if there's greater value from previous steps
	  {
	    zeroRow[c] = pos;
	  }
	  if (zeroCol[pos] < c) // don't overwrite if there's greater value from previous steps
	  {
	    zeroCol[pos] = c;
	  }
	}
      }
    }
  }
}

void Tomonoid::stepE3b(const Element& el, const Element& er, associated_mapset& res)
{
/*
 * 13. For every b ∈ S̄ such that α < b < 1:
– let e ∈ S̄ be such that (b, ε r ) ∼ e,
– if e < b then:
• for every a ∈ S̄ s.t. α < a < ε l and (a, b) ∈ Q:
∗ perform (a, e) ∼
 ̇ (a, b).
 * */
  if (er != Element::top_element) // then (b, er) = (b, 1) = b >= b, nothing to do
  {
    std::shared_ptr<const Element> erPtr = ElementCreator::getInstance().getElementPtr(er);
    for (int b = 1; b < this->size - 1; b++)
    {
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b + 1, this->size + 1);
      const Element& e = getResult(bPtr , erPtr);
      //std::cout << *(bPtr.get()) << " * " << *(erPtr.get()) << " = " << e;
      
      if (e < *( bPtr.get() ) )
      {
	std::shared_ptr<const Element> ePtr;
	if (e == Element::bottom_element)
	{
	  ePtr = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
	}
	else
	{
	  ePtr = ElementCreator::getInstance().getElementPtr(e);
	}
	
	for (int a = 1; a < posInHelpers(el.getOrder()); a++)
	{
	  if (columnQEnds[a] < b) // = (a,b) uz neni v Q a ani dalsi sloupce napravo uz nebudou
	  {
	    break;
	  }
	  else
	  {
	    std::shared_ptr<const Element> aPtr = ElementCreator::getInstance().getElementPtr(a + 1, this->size + 1);
	    const Element& ae = getResult(aPtr, ePtr);
	    TableElement tab(aPtr, bPtr);
	    TableElement tae(aPtr, ePtr);
	    // TODO - rethink it again, but e < b
	    /*
	    if (ae != Element::bottom_element)
	    {
	      std::shared_ptr<const Element> aePtr = ElementCreator::getInstance().getElementPtr(ae);
	      this->nextImportantResults.insert(std::pair<TableElement, std::shared_ptr<const Element>>(tab, aePtr) );
	      // TODO and for every TableElement in Q higher and right to it ???
	    }
	    else
	    {*/
	    
	      associated_mapset::iterator itab = res.find(tab);
	      associated_mapset::iterator itae = res.find(tae);
	      
	      insertAssociated(itab, tab, tae, res);
	      insertAssociated(itae, tae, tab, res);
	    //}
	  }
	}
      }
    }
  }
  

/*14. For every b ∈ S̄ such that α < b < 1:
– let d ∈ S̄ be such that (ε l , b) ∼ d,
– if d < b then:
• for every c ∈ S̄ s.t. α < c < ε r and (b, c) ∈ Q:
∗ perform (d, c) ∼
 ̇ (b, c).*/
  if (el != Element::top_element) // then (el, b) = (1, b) = b >= b, nothing to do
  {
    std::shared_ptr<const Element> elPtr = ElementCreator::getInstance().getElementPtr(el);
    for (int b = 1; b < this->size - 1; b++)
    {
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b + 1, this->size + 1);
      const Element& d = getResult(elPtr, bPtr);
      
      if (d < *( bPtr.get() ) )
      {
	// might be zero!!!
	std::shared_ptr<const Element> dPtr;
	if (d == Element::bottom_element)
	{
	  dPtr = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
	}
	else
	{
	  dPtr = ElementCreator::getInstance().getElementPtr(d);
	}
	
	for (int c = 1; c < posInHelpers(er.getOrder()); c++)
	{
	  if (columnQEnds[b] < c)
	  {
	    break;
	  }
	  else
	  {
	    std::shared_ptr<const Element> cPtr = ElementCreator::getInstance().getElementPtr(c + 1, this->size + 1);
	    const Element& dc = getResult(dPtr, cPtr);
	    TableElement tdc(dPtr, cPtr);
	    TableElement tbc(bPtr, cPtr);
	    // TODO think about it again, but d < b -> (d,c) < (b,c) anyway!
	    /*
	    if (dc != Element::bottom_element)
	    {
	      std::shared_ptr<const Element> dcPtr = ElementCreator::getInstance().getElementPtr(dc);
	      this->nextImportantResults.insert(std::pair<TableElement, std::shared_ptr<const Element>>(tab, aePtr) );
	      // TODO and for every TableElement in Q higher and right to it ???
	    }
	    else
	    {*/
	      associated_mapset::iterator itdc = res.find(tdc);
	      associated_mapset::iterator itbc = res.find(tbc);
	      
	      insertAssociated(itbc, tbc, tdc, res);
	      insertAssociated(itdc, tdc, tbc, res);
	    //}
	  }
	}
      }
    }
  }
}

void Tomonoid::editZerosAndAtoms()
{
  if (zeroCol[this->size - 2] == UNASSIGNED)
  {
    zeroCol[this->size - 2] = NOT_PRESENT;
  }
  int prevCol = zeroCol[this->size - 2];
  if (zeroRow[this->size - 2] == UNASSIGNED)
  {
    zeroRow[this->size - 2] = NOT_PRESENT;
  }
  int prevRow = zeroRow[this->size - 2];
  for (int i = this->size - 3; i >= 0; i--)
  {
      if (zeroCol[i] == UNASSIGNED)
      {
	zeroCol[i] = prevCol;
      }
      if (prevCol != NOT_PRESENT)
      {
	zeroCol[i] = std::max<int>(zeroCol[i], prevCol);
      }      
      prevCol = zeroCol[i];
      
      if (zeroRow[i] == UNASSIGNED)
      {
	zeroRow[i] = prevRow;
      }
      if (prevRow != NOT_PRESENT)
      {
	zeroRow[i] = std::max<int>(zeroRow[i], prevRow);
      }      
      prevRow = zeroRow[i];
  }
  
  
  if (atomCol[0] == UNASSIGNED)
  {
    atomCol[0] = NOT_PRESENT;
  }
  prevCol = atomCol[0];
  
  if (atomRow[0] == UNASSIGNED)
  {
    atomRow[0] = NOT_PRESENT;
  }
  prevRow = atomRow[0];
  
  for (int i = 1; i < this->size - 1; i++)
  {
    if (atomCol[i] == UNASSIGNED)
    {
      atomCol[i] = prevCol;
    }
    if (prevCol != NOT_PRESENT)
      {
	atomCol[i] = std::min<int>(atomCol[i], prevCol);
      }  
    prevCol = atomCol[i];
    
    if (atomRow[i] == UNASSIGNED)
    {
      atomRow[i] = prevRow;
    }
    if (prevRow != NOT_PRESENT)
      {
	atomRow[i] = std::min<int>(atomRow[i], prevRow);
      }  
    prevRow = atomRow[i];
  }
  
}

void Tomonoid::controlFreeValues(associated_mapset& associatedValues)
{
  for (int i = 0; i < this->size - 1; i++)
  {
    if (zeroCol[i] != NOT_PRESENT)
    {  
      for (int j = zeroCol[i] + 1; j < std::min<int>(atomCol[i], columnQEnds[i] + 1); j++)
      {
	std::shared_ptr<const Element> lft = ElementCreator::getInstance().getElementPtr(i + 1, this->size + 1);
	std::shared_ptr<const Element> rgt = ElementCreator::getInstance().getElementPtr(j + 1, this->size + 1);
	TableElement lrgte(lft, rgt);
	if (associatedValues.count(lrgte) == 0)
	{
	  std::set<TableElement> st;
	  st.insert(lrgte);
	  associatedValues.insert(std::pair<TableElement, std::set<TableElement>>(lrgte, st));
	}
      }
    }
  }
}

bool Tomonoid::assignAtom(std::set<std::set<TableElement>*>& ptrset,
			  std::unordered_map< TableElement, std::set< TableElement >* >& telToSet,
			  Tomonoid *nextTomo
			  )
{
  std::shared_ptr<const Element> atomPtr = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
  std::set<TableElement> atomsAssigned;
  // zacneme s nutnyma atomama
  for (int i = 0; i < this->size - 1; i++) // first column
  {
    int min = atomCol[i]; // where atomStarts
    for (int j = min; j <= columnQEnds[i]; j++) //till we're in Q
    {
      std::shared_ptr<const Element> left = ElementCreator::getInstance().getElementPtr(i + 1, this->size + 1);
      std::shared_ptr<const Element> right = ElementCreator::getInstance().getElementPtr(j + 1, this->size + 1);
      TableElement lrte(left, right);
      if (atomsAssigned.count(lrte) == 0)
      {
	std::unordered_map<TableElement, std::set<TableElement>*>::iterator associt = telToSet.find(lrte);
	nextTomo->importantResults.insert(std::pair<TableElement, std::shared_ptr<const Element>>(lrte, atomPtr) );
	atomsAssigned.insert(lrte);
	telToSet.erase(lrte);
	if (associt != telToSet.end() )
	{
	  std::set<TableElement>* assoc_ptr = (*associt).second;
	  for (std::set<TableElement>::iterator ptrit = assoc_ptr->begin(); ptrit != assoc_ptr->end(); ++ptrit)
	  {
	    const TableElement& tepp = *ptrit;
	    const unsigned int left = tepp.getLeft().get()->getOrder();
	    const unsigned int right = tepp.getRight().get()->getOrder();
	    int leftPos = this->size - 1 - left;
	    if (zeroCol[leftPos] >= this->size - 1 - right && zeroCol[leftPos] != NOT_PRESENT) // atom assignment intersects with zero area -> can't create this
	    {
	      #ifdef DEBUG
	      std::cerr << "row " << this->size - 1 - left << " should be less than " << this->size - 1 - right << std::endl;
	      #endif
	      return false;
	    }
	    
	    if (atomsAssigned.count(tepp) == 0)
	    {
	      atomsAssigned.insert(tepp);
	      telToSet.erase(tepp);
	      nextTomo->importantResults.insert(std::pair<TableElement, std::shared_ptr<const Element>>(tepp, atomPtr) );
	    }
	  }
	  ptrset.erase(assoc_ptr);
	//  std::cout << "deleting " << assoc_ptr << std::endl;
	  delete assoc_ptr;
	}
      }
    }
  }
  
  return true;
}

void Tomonoid::markToDelete(std::set<TableElement> *current,
			    std::unordered_set<std::set<TableElement>*> *toBeDeleted,
			    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets
)
{
  if (toBeDeleted->find(current) == toBeDeleted->end())
  {
    toBeDeleted->insert(current);
    auto rev_it = revertSets.find(current);
    if (rev_it != revertSets.end() )
    {
      std::set< std::set< TableElement >* > *value = (*rev_it).second;
      for (auto it = value->begin(); it != value->end(); ++it)
      {
	std::set<TableElement> *next = *it;
	markToDelete(next, toBeDeleted, revertSets);
      }
    }
  }
}

void Tomonoid::assignZeros(std::set<std::set<TableElement>*>& ptrset,
			   std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
			   std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets
)
{
  std::unordered_set<std::set<TableElement>*> *toBeDeleted = new std::unordered_set<std::set<TableElement>*>();
  for (std::set<std::set<TableElement>*>::iterator ejty = ptrset.begin(); ejty != ptrset.end(); ++ejty)
  {
     std::set<TableElement>* setTel = *ejty;
     if (toBeDeleted->find(setTel) != toBeDeleted->end())
     {
       // already marked to be deleted
       continue;
     }
     //std::cout << "Checking setTel " << setTel << std::endl;
     for (std::set<TableElement>::iterator teit = setTel->begin(); teit != setTel->end(); ++teit)
     {
	const TableElement& teref = *teit;
	int left_or = teref.getLeft().get()->getOrder();
	int right_or = teref.getRight().get()->getOrder();
	int left_pos = posInHelpers(left_or);
	int right_pos = posInHelpers(right_or);
	
	if (zeroCol[left_pos] != NOT_PRESENT && zeroCol[left_pos] >= right_pos)
	{
	  // this associated set is in zeroed area
	  // so recursively mark all in revert sets to be deleted
	  markToDelete(setTel, toBeDeleted, revertSets);
	  
	  break;
	}
     }
  }
  
  for (auto it = toBeDeleted->begin(); it != toBeDeleted->end(); ++it)
  {
    std::set<TableElement> *toDelptr = *it;
#ifdef VERBOSE
    std::cout << "We should erase " << toDelptr << std::endl;
#endif
    ptrset.erase(toDelptr);
    
    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator rIt, pIt;
    rIt = revertSets.find(toDelptr);
    
    if (rIt != revertSets.end() )
    {
      delete (*rIt).second;
      revertSets.erase(toDelptr); 
    }
    
    pIt = precededSets.find(toDelptr);
    
    if (pIt != precededSets.end() )
    {
      delete (*pIt).second;
      precededSets.erase(toDelptr);
    }
    
    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator revIt;
    for (revIt = revertSets.begin(); revIt != revertSets.end(); revIt++)
    {
      std::set<std::set<TableElement>*> *sec = (*revIt).second;
      sec->erase(toDelptr);
    }
    
    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >::iterator precIt;
    for (precIt = precededSets.begin(); precIt != precededSets.end(); precIt++)
    {
      std::set<std::set<TableElement>*> *sec = (*precIt).second;
      sec->erase(toDelptr);
    }
    
    delete toDelptr;
  }
  delete toBeDeleted;
}

void Tomonoid::validPermutations(std::vector<Tomonoid*> &res,
			 std::set<std::set<TableElement>*>& ptrset, 
			 Tomonoid *zeroTom,
			 std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* > &precededSets,
			 std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* > &revertSets
				)
{
  int setCount = ptrset.size();
  
  std::shared_ptr<const Element> atomPtr = ElementCreator::getInstance().getElementPtr(1, this->size + 1);
  
  // SO WHAT DO WE NEED?
  // Make random assignation, then check if it is OK
  // so that for all values, if they have something in preceding set, it is <= their value
  // so at first, let's take values from 1 to 2 on K
  // assign them in order of assignVal (ptrset)
  // take some, if it assigned 1, then nothing to check (precedings will be <=)
  // if it is 0, then check if all precedings are also 0
  
  if (setCount <= LOOPING_THRESHOLD) // let's say that until this it might be faster to try and check
  {
	
    res.push_back(zeroTom);
    std::map<std::set<TableElement>*, int> assignedPosition;
  
    int i = 1;
    
    // assign power of 2 for each associated set (so we can check bits) 
    for (std::set<std::set<TableElement>*>::iterator eat = ptrset.begin(); eat != ptrset.end(); ++eat)
    {
      std::set<TableElement> *key_ptr = *eat;
      assignedPosition.insert(std::make_pair(key_ptr,i) );
      i *= 2;
    }
    
    int pow = 1 << setCount; // 2 on K
    
    // starting from 1 (don't check monoid with all sets assigned 0) check if given combination of 1s/0s
    // meets monotonicity requirement
    for (i = 1; i < pow; i++)
    {
      bool ok = true;
      
      // iterate over all associated sets
      std::set<std::set<TableElement>*>::iterator it = ptrset.begin();
      for (it; it != ptrset.end(); ++it)
      {
	std::set<TableElement> *curr_set_ptr = *it;
	std::map<std::set<TableElement>*, int>::iterator ctrit = assignedPosition.find(curr_set_ptr);
	int ctr = (*ctrit).second;
	int assigned = (i & ctr);
	
	std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* >::iterator kk;
	kk = revertSets.find(curr_set_ptr);
	
	if (assigned == 0 &&  kk != revertSets.end() ) // is not empty and not assigned 1
	{ // then check if those values are also zero	
	  std::set<std::set<TableElement>*>::iterator itt = (*kk).second->begin();
	  for (itt; itt != (*kk).second->end(); ++itt)
	  {
	    std::set<TableElement> *control_set_ptr = *itt;
	    std::map< std::set<TableElement>*, int>::iterator pairos = assignedPosition.find(control_set_ptr);
	    
	    int val = (*pairos).second;
	    
	    if (i & val)
	    {
	      ok = false;
	      break;
	    }
	  }
	}
	
	if (!ok)
	{
	  break;
	}
      }
      if (ok) // Current distribution is OK 
      {
	Tomonoid *nxtRes = new Tomonoid(*zeroTom); // COPY CONSTRUCTOR!!
	res.push_back(nxtRes);
	
	std::map<std::set<TableElement>*, int>::iterator nt = assignedPosition.begin();
	for (nt; nt != assignedPosition.end(); ++nt)
	{
	  int bla = (*nt).second;
	  std::set<TableElement> *nvm = (*nt).first;
	  if (bla & i)
	  {
	    for (std::set<TableElement>::iterator nt2 = nvm->begin(); nt2 != nvm->end(); ++nt2)
	    {
	      nxtRes->importantResults.insert(std::make_pair(*nt2, atomPtr));
	    }
	  }
	}
      }
    }
  }
  else // else try to assign through graph
  {   
    #ifdef DEBUG
    std::cerr << "assignThroughGraph" << std::endl;
    #endif
    
    { // so we don't have to allocate it on heap
      StrongConnectivityFinder scf(&ptrset, &precededSets, &revertSets);
      scf.findComponents();
    } // and there it is destructed
    
#ifdef VERBOSE
    std::cerr << "After running scf" << std::endl;
    printAllInfo(ptrset, precededSets, revertSets);
#endif
    
    GraphAssignator ga(&precededSets, &revertSets, zeroTom, &res);
    ga.doAssignment();
    //assignOthers(revertSets, precededSets, ptrset, res, zeroTom, telToSet);
#ifndef CONTROL
    delete zeroTom;
#endif
  }
  
#ifdef CONTROL
  std::vector<Tomonoid*> *control = new std::vector<Tomonoid*>();
  
  {
    StrongConnectivityFinder scf(&ptrset, &precededSets, &revertSets);
    scf.findComponents();
  }
  
  {
    //assignOthers(revertSets, precededSets, ptrset, *control, zeroTom, telToSet);
    GraphAssignator ga(&precededSets, &revertSets, zeroTom, &res);
    ga.doAssignment();
  }
  size_t bla = control->size();
  
  if (bla != res.size() )
  {
    control_print_mutex.lock();
    std::cerr << "Error on this: " << std::endl;
    std::cerr << "Graph: " << bla << ", loop: " << res.size() << std::endl;;
    TomonoidPrinter tp;
    tp.printTomonoid(this);
    control_print_mutex.unlock();
  }
  
  for (auto it = control->begin(); it != control->end(); ++it)
  {
    Tomonoid *t = *it;
    delete t;
  }
  
  if (setCount > LOOPING_THRESHOLD)
  {
    delete zeroTom;
  }
  delete control;
#endif
  
  // DELETING + CONTROL BLOCK
    for (auto iiit = precededSets.begin(); iiit != precededSets.end(); ++iiit)
    {
      std::set<std::set<TableElement>*>* setDusets = (*iiit).second;
      delete setDusets;
    }
    
    for (auto reviii = revertSets.begin(); reviii != revertSets.end(); ++reviii)
    {
      std::set<std::set<TableElement>*>* setDusets = (*reviii).second;
      delete setDusets;
    }
  
  for (auto it = ptrset.begin(); it != ptrset.end(); ++it)
  {
    std::set<TableElement> *setTel = (*it);
    delete setTel;
  }
}

void Tomonoid::calcPrecedings(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
			      std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets,
			      std::set<std::set<TableElement>*>& ptrset
 			    )
{
    
    //Tomonoid *fullOnes = new Tomonoid(*zeroTom); // COPY CONSTRUCTOR!
    //std::set<std::set<TableElement>*>::iterator ity = ptrset.begin();
    for (auto ity = ptrset.begin(); ity != ptrset.end(); ++ity) // for each associated set
    {
      std::set<TableElement>* setTel = (*ity);
      //std::cout << "calcprec: " << setTel << std::endl;
      
      std::set<std::set <TableElement>* > *currentPrecSets = new std::set<std::set <TableElement>* >();
      precededSets.insert(std::make_pair(setTel, currentPrecSets) );
      
      std::set<std::set<TableElement>*>::iterator inner = ptrset.begin();
      
      for (inner; inner != ptrset.end(); ++inner) // take a look on other sets
      {
	std::set<TableElement>* compTel = *inner;
	
	if (compTel == setTel)
	{
	  continue;
	}
	for (std::set<TableElement>::iterator origTels = setTel->begin(); origTels != setTel->end(); ++origTels)
	{ // compare Elements from this set (setTel)
	  const TableElement &first = *origTels;
	  for (std::set<TableElement>::iterator nextTels = compTel->begin(); nextTels != compTel->end(); ++nextTels)
	  { // to those in other set (compTel)
	    const TableElement &second = *nextTels;
	    
	    int fl = first.getLeft().get()->getOrder(), fr = first.getRight().get()->getOrder();
	    int sl = second.getLeft().get()->getOrder(), sr = second.getRight().get()->getOrder();
	    
	    //and if something from orig precedes something from next, orig forces value in next and we break to check others.
	    if (fl >= sl // f must be more left or same column -> higher or equal order 
	      && fr >= sr) // f must be lower or same row -> higher or equal order
	    {
	      currentPrecSets->insert(compTel);
	      
	      std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >* >::iterator 
		    revIt = revertSets.find(compTel);
	      if (revIt == revertSets.end() )
	      {
		std::set<std::set<TableElement>*>* revptr = new std::set<std::set<TableElement>*>();
		revptr->insert(setTel);
		revertSets.insert(std::pair<std::set<TableElement>*, std::set< std::set <TableElement>* >*>(compTel, revptr));
	      }
	      else
	      {
		(*revIt).second->insert(setTel);
	      }
	      
	      goto checkNext; //we can break even outer for loop
	    }
	  }
	}
	checkNext:;
      }
      
    }
  
}


void printAllInfo(std::set<std::set<TableElement>*>& ptrset,
		  std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
		  std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets)
{
    int dd = 0;
    std::cerr << "ptrset:" << std::endl;
  for (std::set<std::set<TableElement>*>::iterator sit = ptrset.begin(); sit != ptrset.end(); ++sit)
  {
    std::cout << "Iteration " << dd << ", address: " << *sit << std::endl;
    for (std::set<TableElement>::iterator eas = (*sit)->begin(); eas != (*sit)->end(); ++eas)
    {
      std::cout << *eas << std::endl;
    }
    dd++;
  }
  
  int kl = 0;
  
  for (std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >*>::iterator sit = precededSets.begin(); 
       sit != precededSets.end(); ++sit)
  {
    std::cerr << "Iteration " << kl << ", address: " << (*sit).first << std::endl;
    std::set<std::set<TableElement>*>* p2 = (*sit).second;
    for (std::set<std::set<TableElement>*>::iterator eas = p2->begin(); eas != p2->end(); ++eas)
    {
      std::cerr << "prec_adres: " << *eas << std::endl;
    }
    kl++;
  }
  kl=0;
  for (std::map< std::set<TableElement>*, std::set< std::set <TableElement>* >*>::iterator sit = revertSets.begin(); 
       sit != revertSets.end(); ++sit)
  {
    std::cerr << "Iteration " << kl << ", address: " << (*sit).first << std::endl;
    std::set<std::set<TableElement>*>* p2 = (*sit).second;
    for (std::set<std::set<TableElement>*>::iterator eas = p2->begin(); eas != p2->end(); ++eas)
    {
      std::cerr << "rev_adres: " << *eas << std::endl;
    }
    kl++;
  }
}
