#include "tomonoid.h"

Tomonoid::Tomonoid() : Tomonoid::Tomonoid(2, NULL) {};

Tomonoid::Tomonoid(unsigned int sz, Tomonoid* prev)
{
  size = sz;
  previous = prev;
  //associatedElements = new all_associated_elements();
  //importantResults = new results_map();
  if (prev != NULL)
  {
    nonarchs = std::vector<std::shared_ptr<const Element>>(prev->nonarchs);
  }
  calculateMaxNonarchimedean();
}

Tomonoid::Tomonoid(unsigned int size) : Tomonoid::Tomonoid(size, NULL) {};

Tomonoid::Tomonoid(const Tomonoid& other)
{
  //associatedElements = new all_associated_elements( *(other.associatedElements) );
  importantResults = results_map( other.importantResults );
  this->size = other.size;
  this->previous = other.previous;
  this->maxNonarchimedean = other.maxNonarchimedean;
  this->nonarchs = std::vector<std::shared_ptr<const Element>>( other.nonarchs );
}
  
Tomonoid::Tomonoid(Tomonoid* previous) : Tomonoid::Tomonoid(previous->size + 1, previous) {};

Tomonoid::~Tomonoid()
{
 // deleteAssociatedResults();
  //deleteResultsMap();
  deleteHelpArrays();
  //deleteNonarchs();
}
  
Tomonoid& Tomonoid::operator=(Tomonoid& other)
{
  if (&other == this)
  {
    return *this;
  }
  else
  {
    //deleteAssociatedResults();
    //deleteResultsMap();
    //deleteNonarchs();
    //associatedElements = new all_associated_elements(*other.associatedElements);
    importantResults = results_map(other.importantResults);
    this->size = other.size;
    this->previous = other.previous;
    this->maxNonarchimedean = other.maxNonarchimedean;
    this->nonarchs = std::vector<std::shared_ptr<const Element>>(other.nonarchs);
    return *this;
  }
}

void Tomonoid::findIdempotents(std::vector< Element >& vec)
{
  Element e = Element::top_element;
  vec.push_back(e); // top (1) element is always idempotent
  
  unsigned int coatom_pos = this->size;
  // first and last elements are 0 and 1 respectively, which are skipped
  // if a * a = 0, then for all b < a: b * b = 0 and we can break the loop
  while (coatom_pos > 2)
  {
    std::shared_ptr<const Element> coatom = ElementCreator::getInstance().getElementPtr(coatom_pos - 2, *this);
    const Element& elc = this->getResult(coatom, coatom);
    if (elc == *(coatom.get())) // element is idempotent in this tomonoid
    {
      vec.push_back(elc);
    }
    --coatom_pos;
    if (elc == Element::bottom_element)
    {
      break;
    }
  }
}

inline unsigned int Tomonoid::calcStart(unsigned int j)
{
  return j > this->maxNonarchimedean ? this->maxNonarchimedean - 1 : j;
}

void Tomonoid::insertAssociated(associated_mapset::iterator& mapped, TableElement& key, TableElement& in_set, associated_mapset& res)
{
  if (mapped != res.end() )
  {
      std::set<TableElement>& itset = (*mapped).second;
      itset.insert(in_set);
  }
  else
  {
      
      std::set<TableElement> itset;
      itset.insert(in_set);
      res.insert(std::pair<TableElement, std::set<TableElement>>(key, itset));
  }
}

bool Tomonoid::checkCommutativity()
{
  for (int a = 1; a < this->size - 1; ++a)
  {
    std::shared_ptr<const Element> aPtr = ElementCreator::getInstance().getElementPtr(a, this->size);
    for (int b = this->size - 2; b > a; --b)
    {
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b, this->size);
      TableElement ab(aPtr, bPtr);
      TableElement ba(bPtr, aPtr);
      const Element& abRes = this->getResult(ab);
      const Element& baRes = this->getResult(ba);
      
      if ( abRes != baRes )
      {
	std::cerr << "Commutativity check:" << std::endl;
	std::cerr << ab << " = " << abRes << ", but " << ba << " = " << baRes << std::endl;
	return false;
      }
    }
  }
  return true;
}

void Tomonoid::setCommutativity(associated_mapset &res)
{
  const unsigned int next_sz = this->size + 1;
  for (int a = 1; a < next_sz - 2; ++a) //no need to check also last column
  {
    std::shared_ptr<const Element> aPtr = ElementCreator::getInstance().getElementPtr(a, next_sz);
    for (int b = a + 1; b <= next_sz - 2; ++b)
    {
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b, next_sz);
      TableElement ab(aPtr, bPtr);
      TableElement ba(bPtr, aPtr);
      if (this->getResult(ab) == Element::bottom_element)
      {
	    associated_mapset::iterator abit = res.find(ab);
	    associated_mapset::iterator bait = res.find(ba);
	    
	    insertAssociated(abit, ab, ba, res);
	    insertAssociated(bait, ba, ab, res);
      }
      else
      {
	break; //monotonicity - all elements above this one are in P so out of interest
      }
    }
  }
}

/*
 * Calculates step 5 of algorithm.
  // step 5
  // 1 in P but can be omitted as it provides no additional information about associativity
  // assoc: for every (a,b), (c,d) in P let: (a,b) = d, (b,c) = e; then (a,e) = (d,c)*/
void Tomonoid::calcAssociatedPs(associated_mapset &res)
{
  int topPos = this->size - 2;
  int knownQColEnd = 0;
  
#ifdef VERBOSE
  if (maxNonarchimedean != topPos + 1)
  {
    std::cerr << "maxNonarch: " << maxNonarchimedean << std::endl;
     
  }
#endif
  
  // TODO skipping 
  for (int a = topPos; a > 0; a--)
  {
    std::shared_ptr<const Element> aPtr = ElementCreator::getInstance().getElementPtr(a, this->size);
    
    int startB = a >= maxNonarchimedean ? maxNonarchimedean - 1 : topPos;
    
    for (int b = startB; b > knownQColEnd; b--)
    {
      std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b, this->size);
      const Element& ab = this->getResult(TableElement(aPtr, bPtr));
      
#ifdef VERBOSE
      std::cout << " checking a = " << a << ", b = " << b << ", ELements: " << *(aPtr.get()) << ", " << *(bPtr.get()) << std::endl;
#endif
      
      if (ab == Element::bottom_element)
      {
	knownQColEnd = b;
	columnQEnds[a] = b;
	break;
      }
      else
      {
	//std::cout << "a = " << *(aPtr.get()) << ", b = " << *(bPtr.get()) << "; ab = " << ab <<  std::endl;
	
	std::shared_ptr<const Element> dPtr = ElementCreator::getInstance().getElementPtr(ab);
	
	for (int c = topPos; c > 0; c--)
	{
	  std::shared_ptr<const Element> cPtr = ElementCreator::getInstance().getElementPtr(c, this->size);
	  const Element& bc = this->getResult(bPtr, cPtr);
	  
	  //std::cout << "c = " << *(cPtr.get()) << ", bc = " << bc << std::endl;
	  
	  if (bc == Element::bottom_element)
	  {
	    break;
	  }
	  
	  std::shared_ptr<const Element> ePtr = ElementCreator::getInstance().getElementPtr(bc);
	  TableElement ae(aPtr, ePtr);
	  TableElement dc(dPtr, cPtr);
	  
	  // and check if these values are not already in P
	  const Element& aeResult = this->getResult(ae);
	  const Element& dcResult = this->getResult(dc);
	  
	  if (aeResult != dcResult)
	  {
	    TomonoidPrinter tp;
	    tp.printTomonoid(this);
	    
	    throw std::logic_error("Associativity violated for a:" + aPtr.get()->toString() + ", b:" + bPtr.get()->toString()
		+ ", c:" +  cPtr.get()->toString() + ". (ab, c) = " + dcResult.toString() + ", (a, bc) = " + aeResult.toString()
		);
	  }
	  else if (aeResult == Element::bottom_element)
	  {
	    associated_mapset::iterator aeit = res.find(ae);
	    associated_mapset::iterator dcit = res.find(dc);
	    
	    insertAssociated(aeit, ae, dc, res);
	    insertAssociated(dcit, dc, ae, res);
	  }
	}
	
      }
    }
  }
  
}

void Tomonoid::setImportantResults(const Tomonoid::results_map& n_results)
{
  //deleteResultsMap();
  this->importantResults = results_map(n_results);
}

void Tomonoid::calculateQs()
{

  rowQEnds[0] = this->size -2;
  if (rowQEnds[this->size - 2] == UNASSIGNED)
  {
    rowQEnds[this->size - 2] = 0;
  }
  
  columnQEnds[0] = this->size - 2;
  if (columnQEnds[this->size - 2] == UNASSIGNED)
  {
    columnQEnds[this->size - 2] = 0;
  }
  
  int prev = columnQEnds[this->size - 2];
  for (int i = this->size - 2; i >= 0; i--)
  {
    if (columnQEnds[i] == UNASSIGNED)
    {
      columnQEnds[i] = prev;
    }
    prev = columnQEnds[i];
  }
  
  for (int i = 0; i < this->size - 1; i++)
  {
    int pos = columnQEnds[i];
   // std::cout << "columnEnds-" << i << " = " << pos << std::endl;
    
    rowQEnds[pos] = i;
  }
  
  prev = rowQEnds[this->size - 2];
  for (int i = this->size - 2; i >= 0; i--)
  {
    if (rowQEnds[i] == UNASSIGNED)
    {
      rowQEnds[i] = prev;
    }
    prev = rowQEnds[i];
    //std::cout << "rowEnds-" << i << " = " << prev << std::endl;
  }
  
}


std::vector<Tomonoid*> Tomonoid::calculateExtensions()
{
  std::vector<Tomonoid*> extensions;
  
  // step 1: find idempotents (and insert them to this prepared vector)
  std::vector<Element> idempotents;
  findIdempotents(idempotents);
  
  deleteHelpArrays();
  columnQEnds = new int[this->size - 1];
  rowQEnds = new int[this->size - 1];
  atomCol = new int[this->size - 1];
  atomRow = new int[this->size - 1];
  zeroCol = new int[this->size - 1];
  zeroRow = new int[this->size - 1];
  
  for (int i = 0; i < this->size - 1; i++)
  {
    rowQEnds[i] = UNASSIGNED;
    columnQEnds[i] = UNASSIGNED;
  }
  
  
  associated_mapset *associatedValues = new associated_mapset(); 
  //std::cout << "findPs" << std::endl;
  calcAssociatedPs(*associatedValues);
  //std::cout << "findPs end" << std::endl;
  if (onlyCommutative) 
  {
    setCommutativity(*associatedValues); 
  }
  
  calculateQs();
  
  // TODO zmergovat values
  
  // step 2: for each pair run extension algorithm
  std::vector<Element>::iterator it1 = idempotents.begin();
  for (it1; it1 != idempotents.end(); ++it1)
  {
    std::vector<Element>::iterator it2 = idempotents.begin();
    for (it2; it2 != idempotents.end(); ++it2)
    {
      //std::cout << "Calculating idempotent pair: " << *it1 << " and "<< *it2 << std::endl;
      calExtFromIdempots(extensions, *it1, *it2, *associatedValues);
    }
    
  }
  
  if (!onlyArchimedean)
  {
    Tomonoid* nonarch = new Tomonoid(this);
    nonarch->setTopNotIdempotent(false);
    extensions.push_back(nonarch);
  }
  
  delete associatedValues;
  
  deleteHelpArrays();
  
  return extensions;
}

void Tomonoid::fakeResults(std::shared_ptr<const Element> smallest, std::shared_ptr<const Element> other)
{
  TableElement lalo(smallest, other);
  std::shared_ptr<const Element> mrkev = ElementCreator::getInstance().getElementPtr(1, *this);
  this->importantResults.insert(std::pair<TableElement, std::shared_ptr<const Element>>
				 (lalo, mrkev));
}  

bool Tomonoid::isTopNotIdempotent() const
{
  return topNotIdempotent;
}

void Tomonoid::setTopNotIdempotent(bool topNotIdem)
{
  if (topNotIdem != this->topNotIdempotent)
  {
    if (topNotIdem) // == true -> we're switching from nonarchimedean case back to archimedean
    {
	this->nonarchs.pop_back();
    }
    else
    {
      std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size);
      this->nonarchs.push_back(atom);
	
    }
    this->topNotIdempotent = topNotIdem;
    calculateMaxNonarchimedean();
  }
}

void Tomonoid::calculateMaxNonarchimedean()
{
  if (!this->topNotIdempotent) // least element IS idempotent
  {
    this->maxNonarchimedean = 1;
  }
  else
  {
    // TODO - slo by rychleji pres this->nonarchs, ale radsi to nebudeme ted pokouset
    if (previous != NULL)
    {
      unsigned int prevnon = previous->maxNonarchimedean;
      this->maxNonarchimedean = prevnon + 1;
    }
    else
    {
      this->maxNonarchimedean = this->size - 1; // jakoze 1
      for (auto it = nonarchs.begin(); it != nonarchs.end(); ++it)
      {
	std::shared_ptr< const Element > narch = *it;
	unsigned int order = this->size - (*narch).getOrder() - 1;
	this->maxNonarchimedean = std::min(order, this->maxNonarchimedean);
	if (order == 1)
	{
	  this->topNotIdempotent = false;
	  break;
	}
      }
    }
  }
}

void Tomonoid::setNonarchimedeanArray(std::vector< std::shared_ptr< const Element > > nonarchs)
{
  this->nonarchs = nonarchs;
  calculateMaxNonarchimedean();
}


const Element& Tomonoid::getResult(const TableElement& element) const
{
  const std::shared_ptr<const Element> left = element.getLeft();
  const std::shared_ptr<const Element> right = element.getRight();
  const std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size);
  if ((*left).getType() == ORDINARY && (*right).getType() == ORDINARY)
  {
    results_map::const_iterator result_it = importantResults.find(element);
    if (result_it != importantResults.end()) // We found result in this tomonoid - just return it
    {
      return *(result_it->second);
    }
    // If it misses - then we have to look to previous tomonoids.
    if (previous != NULL)
    {
      // We have to look further
      if (topNotIdempotent)
      {
	if (*(left.get()) != *(atom.get()) && *(right.get()) != *(atom.get()) )
	// we're not in last column or row
	{
	  return previous->getResult(element);
	}
	else
	{
	  // we're in last column or row => in next tomonoids there cannot be any result for these values
	  // and as they're missing in important results, the result must be 0
	  return Element::bottom_element;
	}
      }
      else
      {
	const Element& lowest = ElementCreator::getInstance().getElement(1, this->size); // atom
	if (*(left.get()) == lowest || *(right.get()) == lowest) // we're in last column or row - it must be atom
	{
	  return lowest;
	}
	else
	{
	  // take a look further, but if it returns 0, we have to change it to atom (lowest)
	  const Element& res = previous->getResult(element);
	  
	  if (res == Element::bottom_element) 
	  {
	    return lowest;
	  } 
	  else
	  {
	    return res;
	  }
	}
      }
    }
    else
    {
      // Calculate result in respect to known archimedeanicities...
      if (nonarchs.empty() )
      {
	if (topNotIdempotent) 
	{
	  return Element::bottom_element;
	}
	else
	{
	  return ElementCreator::getInstance().getElement(1, this->size);
	}
      }
      else
      {
	int orderMax = std::max<int>(left.get()->getOrder(), right.get()->getOrder());
	int nearestBigger = -1;
	for (std::vector<std::shared_ptr<const Element>>::const_iterator it = nonarchs.begin(); it != nonarchs.end(); ++it)
	{
	  std::shared_ptr<const Element> curr = *it;
	  int currord = curr.get()->getOrder();
	  if (currord == orderMax)
	  {
	    return *(curr.get()); // we're lucky and can return
	  }
	  if (currord > orderMax)
	  {
	    if (nearestBigger < 0 || nearestBigger > currord) {
	      nearestBigger = currord;
	    }
	  }
	}
	
	if (nearestBigger < 0)
	{
	  return this->topNotIdempotent ? Element::bottom_element : *(atom.get());
	}
	else
	{
	  int findOrd = this->size - 1 - nearestBigger;
	  const Element &res = ElementCreator::getInstance().getElement(findOrd, this->size);
	  return res;
	}
      }
    }
  }
  else
  {
    switch ((*left).getType())
    {
      case ORDINARY: // anything * BOTTOM = BOTTOM (Right) vs anything * TOP = anything (Left)
      {
	const Element& ret = (*right).getType() == BOTTOM ? *right : *left;
	return ret;
      }
      case BOTTOM: // BOTTOM * anything == BOTTOM
      {
	return *left;
      }
      default: // TOP * anything = anything
      {
	return *right;
      }
    }
  }
}

const Element& Tomonoid::getResult(std::shared_ptr<const Element> left, std::shared_ptr<const Element> right) const
{
  TableElement te(left, right);
  return this->getResult(te);
}

/*void Tomonoid::deleteResultsMap()
{
  results_map::iterator ity = importantResults->begin();
  //std::cout << "deleteResultsMap" << std::endl;
  for (ity; ity != importantResults->end(); ++ity)
  {
    (*ity).second.reset();
  }
  delete importantResults;
}*/

/*void Tomonoid::deleteAssociatedResults()
{
  if (associatedElements != NULL)
  {
    all_associated_elements::iterator it = associatedElements->begin();
    for (it; it != associatedElements->end(); ++it)
    {
      (*it).clear();
    }
    delete associatedElements;
  }
}*/

unsigned int Tomonoid::getMaxNonarchimedean()
{
  return this->maxNonarchimedean;
}

void Tomonoid::deleteHelpArrays()
{
  if (columnQEnds != NULL)
  {
    delete[] columnQEnds;
    columnQEnds = NULL;
  }
  if (rowQEnds != NULL)
  {
    delete[] rowQEnds;
    rowQEnds = NULL;
  }
  if (atomCol != NULL)
  {
    delete[] atomCol;
    atomCol = NULL;
  }
  if (atomRow != NULL)
  {
    delete[] atomRow;
    atomRow = NULL;
  }
  if (zeroCol != NULL)
  {
    delete[] zeroCol;
    zeroCol = NULL;
  }
  if (zeroRow != NULL)
  {
    delete[] zeroRow;
    zeroRow = NULL;
  }
}

/*void Tomonoid::deleteNonarchs()
{
  delete this->nonarchs; // nebo delete[] ?
  // TODO - checknout dokumentaci, jak nakladat s shared_ptr
  this->nonarchs = NULL;
}*/

std::string Tomonoid::saveString(unsigned int id, unsigned int previd)
{
  std::stringstream sstr;
  sstr << "{" << this->size << "," << false << "," << id << "," << previd << ",[";
  /*os << "{\"sz\": " << this->size << ",";
  os << "\"comm\": " << false << ",";
  os << "\"id\": " << id << ",";
  os << "\"previd\": " << previd << ",";
  os << "\"nonarchs\": [" << std::endl;*/
  bool start = true;
  if (!nonarchs.empty())
  {
    if (previd != 0)
    {
      if (!topNotIdempotent)
      {
	sstr << ElementCreator::getInstance().getElement(1, this->size).getOrder();
      }
    }
    else
    {
      std::vector<std::shared_ptr<const Element>>::iterator vit = this->nonarchs.begin();
      
      for (vit; vit != nonarchs.end(); ++vit)
      {
	if (!start)
	{
	  sstr << ",";
	}
	start = false;
	std::shared_ptr<const Element> shp = *vit;
	sstr << shp.get()->getOrder();
      }
    }
  }
  /*os << "]," << std::endl;
  os << "\"vals\": [" << std::endl;*/
  sstr << "],[";
  
  results_map::iterator it = importantResults.begin();
  
  start = true;
  
  if (optimizingSaveMode)
  {
    if (previd != 0)
      {
      int atomNumber = this->size - 2;
      int columnMaxs[atomNumber];
      std::fill_n(columnMaxs, atomNumber, 0);
      for (it; it != importantResults.end(); ++it)
      {
	const std::pair<TableElement, std::shared_ptr<const Element>> &res = *it;
	const TableElement &te = res.first;
	std::shared_ptr<const Element> left = te.getLeft();
	std::shared_ptr<const Element> right = te.getRight();
	int col = this->size - left->getOrder() - 2;
	int rightOrder = right->getOrder();
	int max = columnMaxs[col];
	if (max < rightOrder)
	{
	  columnMaxs[col] = rightOrder;
	}
      }
      
      int control = 0;
      
      for (int i = 0; i < atomNumber; ++i)
      {
	int compare = columnMaxs[i];
	if (compare > control)
	{
	  if (!start)
	  {
	    sstr << ",";
	  }
	  start = false;
	  control = compare;
	  int leftNumber = this->size - i - 2;
	  sstr << "[" << leftNumber << "," << compare << "]";
	  if (control == atomNumber)
	  {
	    break;
	  }
	}
      }
    }
    else
    {
      std::unordered_map<int, int*> vals;
      for (it; it != importantResults.end(); ++it)
      {
	const std::pair<TableElement, std::shared_ptr<const Element>> &res = *it;
	const TableElement &te = res.first;
	std::shared_ptr<const Element> left = te.getLeft();
	std::shared_ptr<const Element> right = te.getRight();
	std::shared_ptr<const Element> result = res.second;
	int rightOrder = right->getOrder();
	int resultOrder = result->getOrder();
	int col = resultOrder - left->getOrder();
	
	int max = 0;
	int *arr;
	auto findIt = vals.find(resultOrder);
	if (findIt == vals.end() )
	{
	  arr = new int[resultOrder]();
	  vals.insert(std::make_pair(resultOrder, arr));
	}
	else
	{
	  arr = (*findIt).second;
	  max = arr[col];
	}
	if (max < rightOrder)
	{
	  arr[col] = rightOrder;
	}
      }
      
      for (auto it = vals.begin(); it != vals.end(); ++it)
      {
	int* arr = (*it).second;
	int resultOrder = (*it).first;
#ifdef DEBUG
	std::cerr << "resultOrder: " << resultOrder << std::endl;
#endif 
	
	int control = 0;
	for (int i = 0; i < resultOrder; ++i)
	{
	  int compare = arr[i];
	  #ifdef DEBUG
	std::cerr << "i: " << i << ", compare: " << compare << std::endl;
#endif 
	  if (compare > control)
	  {
	    if (!start)
	    {
	      sstr << ",";
	    }
	    start = false;
	    control = compare;
	    int leftNumber = resultOrder - i;
#ifdef DEBUG
	    std::cerr << "[" << leftNumber << "," << compare << "," << resultOrder << "]" << std::endl;
#endif
	    sstr << "[" << leftNumber << "," << compare << "," << resultOrder << "]";
	    if (compare == resultOrder) 
	    {
	      break;
	    }
	  }
	}
	
	delete[] arr;
      }
      
    }
  }
  else
  {
    for (it; it != importantResults.end(); ++it)
    {
      if (!start)
      {
	sstr << ",";
      }
      start = false;
      const std::pair<TableElement, std::shared_ptr<const Element>> &res = *it;
      const TableElement &te = res.first;
      std::shared_ptr<const Element> val = res.second;
      std::shared_ptr<const Element> left = te.getLeft();
      std::shared_ptr<const Element> right = te.getRight();
      
      sstr << "[" << left.get()->getOrder() << "," << right.get()->getOrder() << "," << val.get()->getOrder() << "]";
    }
  }
  
  sstr << "]}";
  return sstr.str();
}

void Tomonoid::save(unsigned int id, unsigned int previd, std::ostream& os)
{
  os << saveString(id, previd);
}

bool operator==(const Tomonoid& left, const Tomonoid& right)
{
  if (left.getSize() != right.getSize() || left.isTopNotIdempotent() != right.isTopNotIdempotent())
  {
    return false;
  }
  for (int i = left.getSize() - 2; i > 0; i--)
  {
    std::shared_ptr<const Element> leftEl = ElementCreator::getInstance().getElementPtr(i, left);
    for (int j = left.getSize() - 2; i > 0; i--)
    {
      std::shared_ptr<const Element> rightEl = ElementCreator::getInstance().getElementPtr(i, right);
      TableElement te(leftEl, rightEl);
      const Element& lres = left.getResult(te);
      const Element& rres = right.getResult(te);
      if (lres != rres) return false;
    }
  }
  return true;
}

bool operator!=(const Tomonoid& left, const Tomonoid& right)
{
  return !operator==(left, right);
}
