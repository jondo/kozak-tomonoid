#include "tomonoid.h"

Tomonoid::Tomonoid() : Tomonoid::Tomonoid(2, NULL) {};

Tomonoid::Tomonoid(unsigned int sz, Tomonoid* prev)
{
  size = sz;
  previous = prev;
  associatedElements = new all_associated_elements();
  importantResults = new results_map();
  if (prev != NULL && prev->nonarchs != NULL)
  {
    nonarchs = new std::vector<std::shared_ptr<const Element>>(*(prev->nonarchs));
  }
  calculateMaxNonarchimedean();
}

Tomonoid::Tomonoid(unsigned int size) : Tomonoid::Tomonoid(size, NULL) {};

Tomonoid::Tomonoid(const Tomonoid& other)
{
  associatedElements = new all_associated_elements( *(other.associatedElements) );
  importantResults = new results_map( *(other.importantResults) );
  this->size = other.size;
  this->previous = other.previous;
  this->maxNonarchimedean = other.maxNonarchimedean;
  if (other.nonarchs != NULL)
    {
      this->nonarchs = new std::vector<std::shared_ptr<const Element>>( *(other.nonarchs) );
    }
}
  
Tomonoid::Tomonoid(Tomonoid* previous) : Tomonoid::Tomonoid(previous->size + 1, previous) {};

Tomonoid::~Tomonoid()
{
  deleteAssociatedResults();
  deleteResultsMap();
  deleteHelpArrays();
  deleteNonarchs();
}
  
Tomonoid& Tomonoid::operator=(Tomonoid& other)
{
  if (&other == this)
  {
    return *this;
  }
  else
  {
    deleteAssociatedResults();
    deleteResultsMap();
    deleteNonarchs();
    associatedElements = new all_associated_elements(*other.associatedElements);
    importantResults = new results_map(*other.importantResults);
    this->size = other.size;
    this->previous = other.previous;
    this->maxNonarchimedean = other.maxNonarchimedean;
    if (other.nonarchs != NULL)
    {
      this->nonarchs = new std::vector<std::shared_ptr<const Element>>(*(other.nonarchs));
    }
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
  /*if (j > this->maxNonarchimedean)
  {
    return this->maxNonarchimedean - 1;
  }
  else
  {
    return j;
  }*/
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


/*
 * Calculates step 5 of algorithm.
  // step 5
  // 1 in P but can be omitted as it provides no additional information about associativity
  // assoc: for every (a,b), (c,d) in P let: (a,b) = d, (b,c) = e; then (a,e) = (d,c)*/
void Tomonoid::calcAssociatedPs(associated_mapset &res)
{
  //std::cout << "calcAssociatedPs" << std::endl;
  unsigned int j_start = 1;
  for (unsigned int i = this->size - 2; i > 0; i--)
  {
    //std::cout << "i = " << i << std::endl;
    std::shared_ptr<const Element> iPtr = ElementCreator::getInstance().getElementPtr(i, this->size);
    
    //std::cout << "Odpovida element: " << *iPtr << std::endl;
    
    for (unsigned int j = j_start; j < this->size - 1; j++) // (i, j)
    {
      //std::cout << "j = " << j << std::endl;
      std::shared_ptr<const Element> jPtr = ElementCreator::getInstance().getElementPtr(j, this->size);
      //std::cout << "Odpovida element: " << *jPtr << std::endl;
      const Element& ij = this->getResult(iPtr, jPtr);
      //std::cout << "ij = " << ij << std::endl;
      
      if (ij != Element::bottom_element)
      {
	
	
	for (unsigned int k = calcStart(j); k > 0; k--) // (j, k)
	{
	  
	  //std::cout << "k = " << k << std::endl;
	  
	  std::shared_ptr<const Element> kPtr = ElementCreator::getInstance().getElementPtr(k, this->size);
	  //std::cout << "Odpovida element: " << *kPtr << std::endl;
	  const Element& jk = this->getResult(jPtr, kPtr);
	  //std::cout << "jk = " << jk << std::endl;
	  if (jk != Element::bottom_element)
	  {
	    // (i, jk) = (ij, k)
	    std::shared_ptr<const Element> jkPtr = ElementCreator::getInstance().getElementPtr(jk);
	    std::shared_ptr<const Element> ijPtr = ElementCreator::getInstance().getElementPtr(ij);
	    
	   // std::cout << "jk: " << *jkPtr << std::endl;
	   // std::cout << "ij: " << *ijPtr << std::endl;
	    
	    const Element& ij_Kres = this->getResult(ijPtr, kPtr);
	    const Element& i_Jkres = this->getResult(iPtr, jkPtr);
	    
	    if (ij_Kres == Element::bottom_element && i_Jkres == Element::bottom_element)
	    {
	      TableElement t1(iPtr, jkPtr);
	      TableElement t2(ijPtr, kPtr);
	      associated_mapset::iterator it = res.find(t1);
	      associated_mapset::iterator it2 = res.find(t2);
	      
	      insertAssociated(it, t1, t2, res);
	      insertAssociated(it2, t2, t1, res);
	    }
	    else
	    {
	      // should be equal
	      if (ij_Kres != i_Jkres)
	      {
		TomonoidPrinter tp;
		tp.printTomonoid(this);
		Tomonoid* prev = this->previous;
		while (prev != NULL)
		{
		    tp.printTomonoid(prev);
		    prev = prev->previous;
		}
		
		throw std::logic_error("Associativity violated for i:" + iPtr.get()->toString() + ", j:" + jPtr.get()->toString()
		+ ", k:" +  kPtr.get()->toString() + ". (ij, k) = " + ij_Kres.toString() + ", (i, jk) = " + i_Jkres.toString()
		);
	      }
	      // if they are, then there's nothing to do as the result is already known and can't be changed
	    }
	    
	  }
	  else
	  {
	    break; // we're not in P anymore and all results below this point are also 0
	  }
	}
      }
      else
      {
	// in next row, we don't need to start at first element (first column) but we can move to next from this 
	// one, because  everything before this point is 0 => is not in P
	columnQEnds[i] = j;
	j_start = j + 1;
      }
    }
  }
}

void Tomonoid::setImportantResults(Tomonoid::results_map& n_results)
{
  deleteResultsMap();
  this->importantResults = new results_map(n_results);
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
  /*
  std::cout << "columnQEnds" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cout << i << ": " <<  columnQEnds[i] << std::endl; 
  }
  std::cout << "rowQEnds" << std::endl;
  for (int i = 0; i < this->size - 1; i++)
  {
    std::cout << i << ": " <<  rowQEnds[i] << std::endl; 
  }*/
  
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
  
  
  associated_mapset associatedValues; 
  //std::cout << "findPs" << std::endl;
  calcAssociatedPs(associatedValues);
  //std::cout << "findPs end" << std::endl;
  
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
      calExtFromIdempots(extensions, *it1, *it2, associatedValues);
    }
    
  }
  
  if (!onlyArchimedean)
  {
    Tomonoid* nonarch = new Tomonoid(this);
    nonarch->setArchimedean(false);
    extensions.push_back(nonarch);
  }
  
  deleteHelpArrays();
  
  return extensions;
}

void Tomonoid::fakeResults(std::shared_ptr<const Element> smallest, std::shared_ptr<const Element> other)
{
  TableElement lalo(smallest, other);
  std::shared_ptr<const Element> mrkev = ElementCreator::getInstance().getElementPtr(1, *this);
  this->importantResults->insert(std::pair<TableElement, std::shared_ptr<const Element>>
				 (lalo, mrkev));
}  

bool Tomonoid::isArchimedean()
{
  return archimedean;
}

void Tomonoid::setArchimedean(bool isArchimedean)
{
  if (isArchimedean != this->archimedean)
  {
    if (isArchimedean) // == true -> we're switching from nonarchimedean case back to archimedean
    {
	this->nonarchs->pop_back();
	if (this->nonarchs->empty() )
	{
	  deleteNonarchs();
	}
    }
    else
    {
      std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size);
      if (this->nonarchs == NULL)
      {
	this->nonarchs = new std::vector<std::shared_ptr<const Element>>();
      }
      this->nonarchs->push_back(atom);
	
    }
    this->archimedean = isArchimedean;
    calculateMaxNonarchimedean();
  }
}

void Tomonoid::calculateMaxNonarchimedean()
{
  if (!this->archimedean)
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
      this->maxNonarchimedean = this->size - 1;
      for (int i = 2; i < this->size - 1; i++)
      {
	std::shared_ptr<const Element> bla = ElementCreator::getInstance().getElementPtr(i, *this);
	const Element& res = this->getResult(bla, bla);
	if (res == *(bla.get()))
	{
	  this->maxNonarchimedean = i;
	  break;
	}
      }
    }
  }
}


const Element& Tomonoid::getResult(const TableElement& element) const
{
  const std::shared_ptr<const Element> left = element.getLeft();
  const std::shared_ptr<const Element> right = element.getRight();
  const std::shared_ptr<const Element> atom = ElementCreator::getInstance().getElementPtr(1, this->size);
  if ((*left).getType() == ORDINARY && (*right).getType() == ORDINARY)
  {
    results_map::iterator result_it = importantResults->find(element);
    if (result_it != importantResults->end())
    {
      return *(result_it->second);
    }
    else
    {
      // TODO - prechod mezi archimedovskym do nearchimedovskyho -> velka cisla tam kde maj bejt nuly!
      if (archimedean) 
      {
	if (previous != NULL && *(left.get()) != *(atom.get()) && *(right.get()) != *(atom.get()) )
	{
	  return previous->getResult(element);
	}
	else
	{
	  return Element::bottom_element;
	}
      }
      else // NOT ARCHIMEDEAN
      {
	const Element& lowest = ElementCreator::getInstance().getElement(1, this->size);
	if (*left == lowest || *right == lowest)
	{
	  return lowest;
	}
	
	if (previous != NULL)
	{
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
	else
	{
	  return lowest;
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

void Tomonoid::deleteResultsMap()
{
  results_map::iterator ity = importantResults->begin();
  //std::cout << "deleteResultsMap" << std::endl;
  for (ity; ity != importantResults->end(); ++ity)
  {
    (*ity).second.reset();
  }
  delete importantResults;
}

void Tomonoid::deleteAssociatedResults()
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
}

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

void Tomonoid::deleteNonarchs()
{
  delete this->nonarchs; // nebo delete[] ?
  // TODO - checknout dokumentaci, jak nakladat s shared_ptr
  this->nonarchs = NULL;
}

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
  if (nonarchs != NULL)
  {
    if (previd != 0)
    {
      if (!archimedean)
      {
	sstr << ElementCreator::getInstance().getElement(1, this->size).getOrder();
      }
    }
    else
    {
      std::vector<std::shared_ptr<const Element>>::iterator vit = this->nonarchs->begin();
      
      for (vit; vit != nonarchs->end(); ++vit)
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
  
  results_map::iterator it = importantResults->begin();
  
  start = true;
  
  for (it; it != importantResults->end(); ++it)
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
  
  sstr << "]}";
  return sstr.str();
}

void Tomonoid::save(unsigned int id, unsigned int previd, std::ostream& os)
{
  os << saveString(id, previd);
}


