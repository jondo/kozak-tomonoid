#ifndef _TOMONOID_H
#define _TOMONOID_H

#define UNASSIGNED -1
#define NOT_PRESENT 2147483647

#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_set>

extern bool onlyArchimedean, onlyCommutative, optimizingSaveMode;

/**
 * Enum of element types.
 * BOTTOM is least element of tomonoid (usually denoted as 0).
 * TOP is the greatest elemnt of tomonoid (usually denoted as 1).
 * ORDINARY are all other elements.
 */
enum ElementType {
  BOTTOM, /**< Least element of tomonoid (0). */
  ORDINARY, /**< All elements between 0 and 1. */
  TOP /**< Top element of tomonoid (1). */
};

/** Class representing an element of tomonoid.
 */
class Element
{
  /**
   * Order of element (used for ORDINARY elements only).
   * !!! Used reversely in the application, lowest order (1) means coatom !!!
   * (the second BIGGEST element of tomonoid).
   * (-> lower the value, the greater is actual order of element).
   * Decided so because tomonoids are extended "from right to left" (only
   * smaller elements are added in every extension), so first added element
   * in tomonoid of size 3 is numbered as 1 and it remains the second biggest
   * element ever after. 
   */
  unsigned int order;
  ElementType type; /**< Type of element */
  
public:
  static const Element bottom_element;
  static const Element top_element;
  
  /**
   * Creates new element with specified order and type.
   * \param order Order of element (should start from 1; revert! (lower order -> greater element).
   * \param type Type of element (only ORDINARY should be used, BOTTOM and TOP are already created).
   */
  Element(const unsigned int order, ElementType type);
  Element(const Element&);
  ~Element();
  
  Element& operator=(Element&);
  
  const unsigned int getOrder() const {return order;}
  const ElementType getType() const {return type;}
  
  std::string toString() const;
  
  friend bool operator==(const Element&, const Element&);
  friend bool operator!=(const Element&, const Element&);
  
  friend bool operator<(const Element&, const Element&);
  friend bool operator>(const Element&, const Element&);
  friend bool operator<=(const Element&, const Element&);
  friend bool operator>=(const Element&, const Element&);
  
  friend std::ostream& operator<<(std::ostream&, const Element&);
};

/**
 * Class representing elements of Cartesian product of tomonoid.
 * This is level set representation of tomonoid, S^2 = SxS.
 */
class TableElement
{
  int te_id; /**< ID of table element.*/
  size_t hash_val; /**< Hash value (used in hash maps).*/
  std::shared_ptr<const Element> left, /**< Left multiplier element. */
				 right; /**< Right multiplier element. */
public:
  /**
   * Creates an empty table element.
   * Both left and right elements are NULL.
   */
  TableElement();
  /**
   * Creates TableElement representing product of two elements.
   * \param left left multiplier element
   * \param right right multiplier element
   */
  TableElement(std::shared_ptr<const Element> left, std::shared_ptr<const Element> right);
  TableElement(const TableElement&);
  ~TableElement();
  
  std::shared_ptr<const Element> getLeft() {return left;}
  std::shared_ptr<const Element> getRight() {return right;}
  
  const std::shared_ptr<const Element> getRight() const {return right;}
  const std::shared_ptr<const Element> getLeft() const {return left;}
  
  TableElement& operator=(TableElement&);
  
  const size_t getHash() const;
  
  friend bool operator==(const TableElement&, const TableElement&);
  friend bool operator!=(const TableElement&, const TableElement&);
  
  friend bool operator<(const TableElement&, const TableElement&);
  friend bool operator>(const TableElement&, const TableElement&);
  friend bool operator<=(const TableElement&, const TableElement&);
  friend bool operator>=(const TableElement&, const TableElement&);
  
  friend std::ostream& operator<<(std::ostream&, const TableElement&);

};

// custom specialization of std::hash can be injected in namespace std
// Hash function for TableElements.
namespace std
{
    template<> struct hash<TableElement>
    {
        typedef TableElement argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& s) const
        {
            return s.getHash();
        }
    };
}

class Tomonoid 
{
public: // so we can see them...
  typedef std::vector<TableElement> associated_values;
  typedef std::unordered_map<TableElement, std::shared_ptr<const Element>> results_map;
  typedef std::vector<associated_values> all_associated_elements;
  typedef std::map<TableElement, std::set<TableElement>> associated_mapset;
  
private:
  
  bool leastNotIdempotent = true; /**< Is tomonoid archimedean? */
  unsigned int size; /**< Number of elements (including top and bottom ones). */
  /**
   * Lowest element that is non-archimedean (e*e = e). 
   * 0 means no such element is present (except top one).
   */
  unsigned int maxNonarchimedean;
  Tomonoid *previous; /**< Pointer to previous tomonoid (the one from which this one was created.*/
  
  //all_associated_elements *associatedElements;
  /**
   * Map of non-zero results for ORDINARY elements.
   * Consists of pairs TableElement : result. If this tomonoid has a pa
   */
  results_map importantResults;
  
  // DENOTING Q AS PART OF TOMONOID WITH NON-ZERO RESULTS.
  
  int *columnQEnds = NULL; // max index of non-0 element
  int *rowQEnds = NULL;
  
  std::vector<std::shared_ptr<const Element>> nonarchs;
  
  // first index of atom
  int *atomRow = NULL;
  int *atomCol = NULL;
  
  // last index of zero
  int *zeroRow = NULL;
  int *zeroCol = NULL;
  
  int *corners = NULL;
  
  int leftBeginning = 0;
  
  inline unsigned int posInHelpers(unsigned int order);
  
  Tomonoid(unsigned int, Tomonoid*);
  
  //void deleteResultsMap();
  //void deleteAssociatedResults();
  void deleteHelpArrays();
  
  void setCommutativity(Tomonoid::associated_mapset& res);
  
  void validPermutations(std::vector< Tomonoid* >& res, 
			 std::set< std::set< TableElement >* >& ptrset,
			 Tomonoid* zeroTom, 
			 std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
			 std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets
			);
  
  void checkVals(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
		 std::vector< Tomonoid* >& res, 
		 Tomonoid* primary, 
		 std::unordered_map< TableElement, std::set< TableElement >* >& telToSet, 
		 std::set< std::set< TableElement >* >& cornerSets, 
		 std::set< std::set< TableElement >* >& lockSets
	      );
  
  bool assignAtom(std::set<std::set<TableElement>*>&,
		  std::unordered_map< TableElement, std::set< TableElement >* >&,
		  Tomonoid*);
  
  void findIdempotents(std::vector<Element>&);
  
  void calExtFromIdempots(std::vector< Tomonoid* >& res, 
			  const Element& el, 
			  const Element& er, 
			  Tomonoid::associated_mapset associatedValues // Must be copied for all idempotents separately!!!
  ); 
  
  void calculateQs();
  void editZerosAndAtoms();
  void calcPrecedings(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
		      std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets,
		      std::set<std::set<TableElement>*>& ptrset
 		    );
  
  void controlFreeValues(associated_mapset&);
  
  void calcAssociatedPs(associated_mapset&);
  inline unsigned int calcStart(unsigned int);
  
  void insertAssociated(associated_mapset::iterator&, TableElement&, TableElement&, associated_mapset&);
  
  void stepE4(const Element&, const Element&, Tomonoid*);
  void stepE3a(const Element&, const Element&);
  void stepE3c(const Element&, const Element&);
  void stepE3b(const Element&, const Element&, associated_mapset&, Tomonoid*);
  
  void calculateMaxNonarchimedean();
  void mergeAssociatedValues(associated_mapset& associatedValues, 
			     std::unordered_multimap< std::set< TableElement >*, TableElement >& setToTel, 
			     std::unordered_map< TableElement, std::set< TableElement >* >& telToSet);
  //void deleteNonarchs();
  
  void assignZeros(std::set< std::set< TableElement >* >& ptrset,
		   std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
		   std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets);
  
  void rebuildPreceded(std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& precededSets, 
		       std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets);
  
  void markToDelete(std::set< TableElement >* current, 
		    std::unordered_set< std::set< TableElement >* >* toBeDeleted, 
		    std::map< std::set< TableElement >*, std::set< std::set< TableElement >* >* >& revertSets);
  
  void nArchAssocCheck();
  bool controlAssociativity(const Element& el, const Element& er, associated_mapset& res, Tomonoid* next);
  
  class GraphAssignator 
  {
    typedef std::set<TableElement> associated_set;
    typedef std::map< associated_set*, std::set< associated_set* >* > mapset_type;
    typedef std::unordered_set<std::set<TableElement>*> assigned_sets;
    
    Tomonoid *primary;
    mapset_type *precededSets;
    mapset_type *revertSets;
    std::vector<Tomonoid*> *res;
    
    std::unordered_map< associated_set*, unsigned int > sizes_map;
    std::shared_ptr< const Element> atom;
    assigned_sets takenSet;
    
    void assignRecursively(std::stack<associated_set*> *zeros, Tomonoid *nowtom);
    
    void assignAtoms(associated_set *current, results_map *rm, assigned_sets *newlyAssignedSets);
    
    void recount(Tomonoid::GraphAssignator::associated_set* current, std::stack<associated_set*> *zeros);
    
    void recountBack(associated_set *current);
    
  public:
    GraphAssignator(mapset_type *precs, 
		    mapset_type *revs, 
		    Tomonoid *prims,
		    std::vector<Tomonoid*> *results
 		  );
    
    void doAssignment();
  };
  
  class StrongConnectivityFinder
  {
    typedef std::set< TableElement > vertex;
    
    std::set< vertex* > *ptrset;
    std::map< vertex*, std::set< vertex* >* > *precededSets;
    std::map< vertex*, std::set< vertex* >* > *revertSets;
    
    std::stack<vertex*> stack;
    std::unordered_set<vertex*> onStack;
    std::unordered_map<vertex*, int> indices;
    std::map<vertex*, std::set<vertex*>*> remap; // REPRESENTING SET -> SET OF SETS IN COMPONENT
    std::map<vertex*, vertex*> newClass; // ORIGINAL SET -> NEW SET (representing whole component) (reverse of remap)
    
    int index = 0;
    
    void tarjan();
  
    int strongConnect(vertex *v);
    
    void rebuildVerticesSets();
    
    void renameValues(std::map<vertex*,std::set<vertex*>*> *curr_set);

  public:
    void findComponents();
    
    StrongConnectivityFinder(std::set< vertex* > *ptrset,
			     std::map< vertex*, std::set< vertex* >* > *precededSets,
			     std::map< vertex*, std::set< vertex* >* > *revertSets);
  };

  
public:
  Tomonoid();
  Tomonoid(unsigned int);
  Tomonoid(const Tomonoid&);
  Tomonoid(Tomonoid*);
  ~Tomonoid();
  
  bool checkCommutativity();
  
  void setNonarchimedeanArray(std::vector<std::shared_ptr<const Element>>);
  
  const std::vector<std::shared_ptr<const Element>>& getNonarchimedeanArray() const {return this->nonarchs;};
  
  std::vector< Tomonoid* >* calculateExtensions();
  
  const results_map& getResults() const {return this->importantResults;};
  
  void save(unsigned int id, unsigned int previd, std::ostream& os);
  
  std::string saveString(unsigned int id, unsigned int previd);
  
  void setImportantResults(const results_map&);
  
  Tomonoid& operator=(Tomonoid&);

  void fakeResults(std::shared_ptr<const Element>, std::shared_ptr<const Element>);
  
  unsigned int getSize() const {return size;}
  
  void setLeastNotIdempotent(bool);
  
  bool isLeastNotIdempotent() const;
  
  unsigned int getMaxNonarchimedean();
  
  // narocne a mozna nepotrebne
  //friend bool operator==(const Tomonoid&, const Tomonoid&);
  //friend bool operator!=(const Tomonoid&, const Tomonoid&);
  
  const Element& getResult(std::shared_ptr<const Element>, std::shared_ptr<const Element>) const;
  
  const Element& getResult(const TableElement&) const;
  
  friend bool operator==(const Tomonoid&, const Tomonoid&);
  friend bool operator!=(const Tomonoid&, const Tomonoid&);

};

/**
 * Class responsible for creating tomonoids from files.
 */
class TomonoidReader
{
private:
  const std::string *str = NULL;
  
  Tomonoid* buildTomonoid(const std::string&);
  
public:
  /**
   * Finds tomonoid representation of tomonoid with given ID.
   */
  Tomonoid* readId(unsigned int id);
  std::vector<Tomonoid*> readSizes(unsigned int size);
  void setString(const std::string *str);
  
  TomonoidReader();
  TomonoidReader(const std::string *str) : str(str) {};

};

/**
 * Singleton class repsonsible for creating Elements.
 */
class ElementCreator
{

  typedef std::vector<std::shared_ptr<const Element>> elements_vector;

  static const unsigned int DEFAULT_SIZE = 512;
  
  unsigned int size;
  
  ElementCreator();  
  ~ElementCreator();
  elements_vector *elementsArray;
  
public:
  static ElementCreator& getInstance();
  
  void operator=(ElementCreator&) = delete;
  ElementCreator(const ElementCreator&) = delete;
  
  const Element& getElement(unsigned int position, unsigned int tomonoidSize);
  const Element& getElement(unsigned int position, const Tomonoid& tomonoid);
  
  // obsahuje assert, ze nevolame bottom/top element, ptz na ty shared_ptry nejsou
  // a ani nesmi byt...
  std::shared_ptr<const Element> getElementPtr(unsigned int, unsigned int);
  std::shared_ptr<const Element> getElementPtr(unsigned int, const Tomonoid&);
  
  std::shared_ptr<const Element> getElementPtr(const Element&);
  
  void enlarge();
};

/**
 * Pretty printer of tomonoids into console.
 */
class TomonoidPrinter
{
public:
  void printTomonoid(Tomonoid *tomo);
};
#endif
