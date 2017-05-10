#include "tomonoid.h"
#include <regex>

const std::string size_start("\\{");
const std::string size_end(",([0-9]+,){3}\\[[0-9,]*\\],(\\[(\\[[0-9]*,[0-9]*,[0-9]*\\],)*(\\[[0-9]*,[0-9]*,[0-9]*\\])|\\[)\\]\\}");

const std::string id_start("\\{([0-9]+,){2}");
const std::string id_end(",[0-9]+,\\[[0-9,]*\\],(\\[(\\[[0-9]*,[0-9]*,[0-9]*\\],)*(\\[[0-9]*,[0-9]*,[0-9]*\\])|\\[)\\]\\}");

const std::string delim_comma(",");
const std::string delim_leftbr("[");
const std::string delim_rightbr("]");
const int delim_len = delim_comma.length();

// pro hledani podle velikosti: 
//  \{5,([0-9]+,){3}\[[0-9,]*\],(\[(\[[0-9]*,[0-9]*,[0-9]*\],)*(\[[0-9]*,[0-9]*,[0-9]*\])|\[)\]\}
// pro hledani podle id: 
Tomonoid* TomonoidReader::readId(unsigned int id)
{
  // TODO takhle to nepujde, nekolikanasobne prochazeni fajlu...
  std::string regex_str = id_start + std::to_string(id) + id_end;
  std::regex reg(regex_str, std::regex::nosubs);
  std::smatch sm;
  std::regex_search(*str, sm, reg);
  
  //std::cerr << "Using regex: " << regex_str << std::endl;
  
  /*std::smatch::iterator it = sm.begin();
  
  for (it; it != sm.end(); ++it)
  {
    static int i = 0;
    std::cerr << i << ": " << *it << std::endl;
    ++i;
  }*/
  
  if (sm.size() == 0)
  {
    std::cerr << "No match for tomonoid ID " << id << std::endl;
    throw std::logic_error("No such ID.");
  }
  if (sm.size() != 1)
  {    
    std::cerr << "More than one match for tomonoid ID " << id << std::endl;
    throw std::logic_error("Duplicate IDs.");
  }
  Tomonoid *tres = buildTomonoid(sm.str(0));
  return tres;
}

std::string nextDelim(std::string& parsed, const std::string& delimiter)
{
  int pos = parsed.find(delimiter);
  std::string token = parsed.substr(0, pos);
  parsed.erase(0, pos + delim_len);
  
  return token;
}

int remap(int size, int num)
{
  return size - num - 1;
}

Tomonoid* TomonoidReader::buildTomonoid(const std::string& sub)
{
  #ifdef DEBUG
  std::cerr << sub << std::endl;
  #endif
  
  std::string working = sub.substr(1); // We don't need starting [ bracket.
  
  //Not checking boundaries etc. - this is private method and sub should conform to regex.
  //Create base tomonoid with size
  std::string token = nextDelim(working, delim_comma);
  int size = std::stoi(token);
  Tomonoid *ret = new Tomonoid(size);
  
  //Commutativity
  // TODO - when it works...
  token = nextDelim(working, delim_comma);
  int comm = std::stoi(token);
  
  // Current tomonoid ID - not needed.
  nextDelim(working, delim_comma);
  
  //Previous tomonoid
  std::unordered_map<TableElement, std::shared_ptr<const Element>> results_map;
  token = nextDelim(working, delim_comma);
  int previd = std::stoi(token);
  std::vector<std::shared_ptr<const Element>> nonarchs;
  if (previd)
  {
    Tomonoid *prev = readId(previd);
    results_map = prev->getResults();
    nonarchs = std::vector<std::shared_ptr<const Element>>(prev->getNonarchimedeanArray());
    delete prev; // not needed anymore
  }
  
  // Nonarchs
  token = nextDelim(working, delim_rightbr);
  if (token.length() > 1) // Non-archimedean vals present.
  {
    int pos;
    token.erase(0,1); // [ Opening bracket
    std::string help; // If there's exactly one nonarch
    
    while (pos = token.find(delim_comma) != std::string::npos)
    {
      help = token.substr(0, pos);
      token.erase(0, pos + 1);
      int val = std::stoi(help);
      nonarchs.push_back(ElementCreator::getInstance().getElementPtr(remap(size,val), *ret));
    }
    help = token;
    pos = std::stoi(help); // And one more number that isn't ended with comma
    
    #ifdef DEBUG
    std::cerr << pos << std::endl;
    #endif
    
    nonarchs.push_back(ElementCreator::getInstance().getElementPtr(remap(size,pos), *ret));
  }
  ret->setNonarchimedeanArray(nonarchs);
  working.erase(0,1); // ,[ in string
  
  token = nextDelim(working, delim_rightbr);
  
  #ifdef DEBUG
  std::cerr << token << std::endl;
  #endif
  
  std::set<TableElement> controlElements;
  
  std::string help;
    while (token.length() > 1)
    {
      token.erase(0,2);
      
      #ifdef DEBUG
      std::cerr << token << std::endl;
      #endif
      
      help = nextDelim(token, delim_comma);
      int left = std::stoi(help);
      
      help = nextDelim(token, delim_comma);
      int right = std::stoi(help);
      
      TableElement te(ElementCreator::getInstance().getElementPtr(remap(size, left), *ret),
		      ElementCreator::getInstance().getElementPtr(remap(size, right), *ret)
      );
      
      int op_res = std::stoi(token);
      #ifdef DEBUG
      std::cerr << "Left: " << left << ", right: " << right << " = " << op_res << std::endl;
      #endif
      
      results_map.insert(std::make_pair(te, ElementCreator::getInstance().getElementPtr(remap(size, op_res), *ret)));
      
      controlElements.insert(te);
      
      token = nextDelim(working, delim_rightbr);
    }
  
  for (auto it = controlElements.begin(); it != controlElements.end(); ++it)
  {
    const TableElement &te = *it;
#ifdef DEBUG
    std::cerr << "Control TableElement ! " << te << std::endl;
#endif
    int col = te.getLeft()->getOrder();
    int row = te.getRight()->getOrder();
    
#ifdef DEBUG
    std::cerr << "col,row: " << col << ", " << row << std::endl;
#endif
    
    auto ti = results_map.find(te);
    std::shared_ptr<const Element> res = (*ti).second;
    
    int jmin = 0;
    for (int i = col; i > 0; --i)
    {
      std::shared_ptr<const Element> leftPtr = ElementCreator::getInstance().getElementPtr(remap(size, i), *ret);
#ifdef DEBUG
      std::cerr << "i = " << i << std::endl;
#endif
      for (int j = row; j > jmin; --j)
      {
	if (i == col && j == row) continue;
	#ifdef DEBUG
        std::cerr << "j = " << j << std::endl;
#endif
	std::shared_ptr<const Element> rightPtr = ElementCreator::getInstance().getElementPtr(remap(size, j), *ret);
	TableElement te2(leftPtr, rightPtr);
#ifdef DEBUG
	std::cerr << te2 <<  std::endl << "result: " << ret->getResult(te2) << std::endl;
#endif
	if (results_map.find(te2) == results_map.end() && ret->getResult(te2) == Element::bottom_element)
	{
#ifdef DEBUG
	  std::cerr << "adding it" << std::endl;
#endif
	  results_map.insert(std::make_pair(te2, res));
	} 
	else
	{
#ifdef DEBUG
	    std::cerr << "jmin = " << j << std::endl;
#endif
	  jmin = j;
	  break;
	}
      }
      if (jmin == row)
      {
#ifdef DEBUG
	std::cerr << "jmin = row = " << row << std::endl;
#endif
	break;
      }
    }
  }
  
  // Results
  ret->setImportantResults(results_map);
  
  return ret;
}

std::vector<Tomonoid*> TomonoidReader::readSizes(unsigned int size)
{
  std::vector<Tomonoid*> res;
  
  return res;
}