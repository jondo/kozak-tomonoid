#include "tomonoid.h"
#include <cmath>
#include <iomanip>
#include "permutation_try.h"
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <stack>
#include <condition_variable>
#include <algorithm>
#include <fstream>
#include <queue>

#define NEW_CALLS 1
#define WAIT 0
#define ALL_DONE 2

bool onlyArchimedean = false, onlyCommutative = false, optimizingSaveMode = false;

std::string outputName = "";

std::vector<Tomonoid> nonmults;
std::vector<Tomonoid> mults;

std::map<unsigned int, unsigned int> levelTomos;

int startLevel = 2;
int maxLevels = 10;

std::mutex stack_mutex;
std::mutex next_id_mut;
std::mutex wait_mut;
std::mutex signal_mut;
std::mutex del_check_mut;
std::mutex write_buf_mut;
std::mutex file_writing_mut;
std::recursive_mutex end_reached_mut;

std::vector<std::string> toWrite; /*< Writing buffer for output file.*/

unsigned int created = 0, deleted = 0;

unsigned int nof_threads = 1;
unsigned int next_id = 1;
/**
 * Number of waiting threads -> when it reaches nof_threads we're done.
 */
unsigned int waiting_threads = 0;
std::condition_variable cv;

struct TomoCount {
  Tomonoid* tomo; /*< Pointer to tomonoid.*/
  TomoCount* previous = NULL; /*< Parent (to be notified when this object is deleted).*/
  unsigned int count; /*< Count of extensions. */
  unsigned int id; /*< ID assigned to Tomonoid (and used when saving). */
  
  /**
   * Coded for use in priority queue.
   * Larger tomonoids with lesser ID should be on top in order to best approximate
   * depth first search.
   */
  inline bool operator<(TomoCount& other)
  {
    return this->tomo->getSize() < other.tomo->getSize() ? true : this->id > other.id;
  }
};

std::unordered_map<TomoCount*, unsigned int> deleteChecker;

struct NextCall 
{
  int iter;
  TomoCount* currentTomo;
  
  NextCall(TomoCount* tomo, int iter)
  {
    this->currentTomo = tomo;
    this->iter = iter;
  };
  
  NextCall(const NextCall& other)
  {
    this->currentTomo = other.currentTomo;
    this->iter = other.iter;
  }
  
  NextCall& operator=(NextCall& other)
  {
    if (this == &other)
    {
      return *this;
    }
    else
    {
      this->currentTomo = other.currentTomo;
      this->iter = other.iter;
      return *this;
    }
  }
  
  inline bool operator<(NextCall& other)
  {
    return *(this->currentTomo) < *(other.currentTomo);
  }
  
  inline bool operator>=(NextCall& other)
  {
    return !operator<(other);
  }
  
};

//std::stack<NextCall*> tomo_stack;
std::priority_queue<NextCall*> tomo_stack;

void calcNextCall(NextCall *nc);

void checkSave(std::ostream& os)
{
  write_buf_mut.lock();
  
  std::vector<std::string> strvec;
  if (toWrite.size() > 200)
  {
    strvec.swap(toWrite);
    write_buf_mut.unlock();
    
    file_writing_mut.lock();
    for (std::vector<std::string>::iterator it = strvec.begin(); it != strvec.end(); ++it)
    {
      os << *it;
    }
    file_writing_mut.unlock();
  }
  else 
  {
    write_buf_mut.unlock();
  }
}

void nthread_fct(int i, std::ostream& os)
{
  std::chrono::milliseconds wait_duration{25};
  unsigned int iter = 1;
  while (true)
  {
    if (iter % 1024 != 0)
    {
        stack_mutex.lock();
	if (!tomo_stack.empty())
	{
	  NextCall *nc = tomo_stack.top();
	  tomo_stack.pop();
	  stack_mutex.unlock();
	  calcNextCall(nc);
	  //std::cout << "Next call " << i << std::endl;
	}
	else
	{
	  stack_mutex.unlock();
	  
	  wait_mut.lock();
	  //std::cout << "Waiting_threads " <<  i << " = " << waiting_threads << std::endl;
	  waiting_threads++;
	  if (waiting_threads >= nof_threads)
	  {
	    //std::cout << "Breaking bad " << i << std::endl;
	    wait_mut.unlock();
	    cv.notify_all();
	    break;
	  }
	  wait_mut.unlock();
	  
	  //std::cout << "Thread " <<  i << " Before wait"<< std::endl;
	  
	  checkSave(os);
	  
	  std::unique_lock<std::mutex> lk(signal_mut);
	  cv.wait_for(lk, wait_duration); 
	  
	  //std::cout << "Thread " <<  i << " waited"<< std::endl;
	  
	  wait_mut.lock();
	  waiting_threads--;
	  wait_mut.unlock();
	}  
    }
    else
    {
      checkSave(os);
    }
    iter++;
  }
}

void endReached(TomoCount *toc)
{
  end_reached_mut.lock();
  TomoCount *const prev = toc->previous;
  
  Tomonoid *const currTomo = toc->tomo;
  
  // SAVING NOW JUST BEFORE PROCESSING TOMONOID
  /*std::string toc_res = currTomo->saveString(id, previd);
  
  write_buf_mut.lock();
  toWrite.push_back(toc_res);
  write_buf_mut.unlock();*/
  
  del_check_mut.lock();
  deleteChecker.erase(toc);
  deleted++;
  del_check_mut.unlock();
  
  delete currTomo;
  delete toc;
  
  if (prev != NULL) {
    del_check_mut.lock();
    deleteChecker[prev]--;
    unsigned int val = deleteChecker[prev];
    del_check_mut.unlock();
    
    if (val == 0 && prev != NULL)
    {
      endReached(prev);
    }
  }
  
  end_reached_mut.unlock();
}

unsigned int finalized = 0;

void calcNextCall(NextCall *nc)
{
  // SAVE PROCESSED TOMONOID
  Tomonoid *curr_tomo = nc->currentTomo->tomo;
  TomoCount *const prev = nc->currentTomo->previous;
  unsigned int prev_id = prev == NULL ? 0 : prev->id;
  std::string toc_res = curr_tomo->saveString(nc->currentTomo->id, prev_id);
  
  write_buf_mut.lock();
  toWrite.push_back(toc_res);
  write_buf_mut.unlock();
  
  if (nc->iter == maxLevels) 
  {
    finalized++;
    //Tomonoid compare(*la);
    //mults.push_back(compare);
    //TomonoidPrinter tp;
    endReached(nc->currentTomo);
    delete nc;
    return;
  }
  // Calculate extensions
  std::vector<Tomonoid*> next_exts = curr_tomo->calculateExtensions();
  
  // Get IDs for saving
  next_id_mut.lock();
  unsigned int newid = next_id;
  next_id += next_exts.size();
  next_id_mut.unlock();
  
  // Remember tomonoid for deleting
  TomoCount *tc = nc->currentTomo;
  tc->count = next_exts.size();
  
  del_check_mut.lock();
  
  unsigned int sz = next_exts.size();
  unsigned int tosz = curr_tomo->getSize() + 1;
  levelTomos[tosz] += sz;
  
  deleteChecker.insert(std::make_pair(tc, tc->count));
  del_check_mut.unlock();
  
  // Push all tomonoids to queue (Don't forget to lock it)
  for (std::vector<Tomonoid*>::iterator it = next_exts.begin(); it != next_exts.end(); ++it)
  {
    created++;
    TomoCount *newcount = new TomoCount();
    newcount->tomo = *it;
    newcount->id = newid;
    newcount->previous = nc->currentTomo;
    // and count will be set in next iteration
    NextCall *next_nc = new NextCall(newcount, nc->iter + 1);
    ++newid;
    stack_mutex.lock();
    tomo_stack.push(next_nc);
    stack_mutex.unlock();
  }
  
  delete nc;
  
  cv.notify_all(); // signal to possible waiters to wake
}

void readArgs(int, char**);

bool assocTest(Tomonoid* t);

unsigned int calcNext(Tomonoid *next, int iter, TomonoidPrinter &tp, std::ostream& os, unsigned int id, unsigned int previd)
{
  next->save(id, previd, os);
  unsigned int nextId = id + 1;
  if (iter == maxLevels) 
  {
    //Tomonoid compare(*next); 
    //nonmults.push_back(compare);
    if (!assocTest(next) )
    {
      std::cout << "Tested was" << std::endl;
      TomonoidPrinter tp;
      tp.printTomonoid(next);
    }
    finalized++;
    return nextId;
  }
  std::vector<Tomonoid*> nextos = next->calculateExtensions();
  //vecOfVec.insert(nextos);
  unsigned int sz = nextos.size();
  unsigned int tosz = next->getSize() + 1;
  levelTomos[tosz] += sz;
  
  for (std::vector<Tomonoid*>::iterator it = nextos.begin(); it != nextos.end(); ++it)
  {
    nextId = calcNext(*it, iter + 1, tp, os, nextId, id);
    delete *it;
  }
  return nextId;
}

void thread_fct(Tomonoid *next)
{
  TomonoidPrinter tp;
  calcNext(next, 1, tp, std::cout, 1, 0);
  delete next;
}

void begin(Tomonoid *t1, std::ostream& os, int threads)
{
  if (threads < 1) {
    threads = 1;
  }
  TomoCount *tc = new TomoCount();
  tc->id = 1;
  next_id++;
  tc->tomo = t1;
  
  NextCall *nc = new NextCall(tc, 1);
  tomo_stack.push(nc);
  
  nof_threads = threads;
  
  std::thread ts[threads];
  
  for (int i = 0; i < threads; i++)
  {
    ts[i] = std::thread(nthread_fct, i + 1, std::ref(os));
  }
  
  for (int i = 0; i < threads; i++)
  {
    ts[i].join();
  }
}

Tomonoid* createt9Tomo();
Tomonoid* create7tomo();

bool multi = false;
bool input = false;
bool input_type_levels = true;
bool input_max_levels = true;
int input_type_arg = 0;

std::string input_name;

void setOutputName();


Tomonoid* control_reader()
{
  
  std::ifstream fs;
  fs.open(input_name, std::ifstream::binary);
  
  fs.seekg(0, fs.end);
  size_t size = fs.tellg();
  std::string buffer(size, ' ');
  fs.seekg(0);
  fs.read(&buffer[0], size); 
  
  TomonoidReader tr(&buffer);
  Tomonoid *t = tr.readId(input_type_arg);
  
  #ifdef DEBUG
  TomonoidPrinter tp;
  tp.printTomonoid(t);
  #endif
  
  #ifdef VERBOSE // check out hash function
  const Tomonoid::results_map &container = t->getResults();
  unsigned nbuckets = container.bucket_count();

  std::cerr << "Results map has " << nbuckets << " buckets:\n";

  for (unsigned i=0; i<nbuckets; ++i) {
    std::cout << "bucket #" << i << " has " << container.bucket_size(i) << " elements.\n";
  }
  #endif
  
  return t;
}

bool help = false;

/*
 * Possible arguments.
 * -c = only commutative
 * -a = only Archimedean
 * -o (filename) = name of output filename
 * -i (filename) [-l (level)|-id (tomonoid_id)] = name of input file + starting level/tomonoid_id (default max level)
 * -max (number) = maximum elements (default to like 20?)
 */
int main(int argc, char **argv) {
  
  readArgs(argc, argv);
  
  if (help == true) return 0;
  
  TomonoidPrinter tp;
  
  //Tomonoid *t7 = createt9Tomo();
  //Tomonoid *t6 = create7tomo();
  
  if (outputName.empty())
  {
    setOutputName();
    std::cerr << "Output file not set, results will be saved to \"" + outputName + "\"." << std::endl;
  }
  
  Tomonoid *beginningTmo;
  if (input)
  {
    beginningTmo = control_reader();
  }
  else
  {
    //beginningTmo = createt9Tomo(); // treba
    beginningTmo = new Tomonoid();
  }
  
  std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
  
  unsigned int hardware_concurr = std::thread::hardware_concurrency();  
  
  #ifdef DEBUG
  std::cout << "So concurr is " << hardware_concurr << std::endl;
  #endif
  
  std::fstream fs(outputName, std::fstream::out);
  
  if (!multi)
  {
    std::cerr << "Running only in one thread (use -multi)" << std::endl;
    begin(beginningTmo, fs, 1);
  }
  else
  {
    std::cerr << "Running multi-threads" << std::endl;
    begin(beginningTmo, fs, hardware_concurr);
  }
  
  std::vector<std::string>::iterator it = toWrite.begin();
  
  for (it; it != toWrite.end(); ++it)
  {
    //std::cout << *it << std::endl;
    fs << *it;
  }
  
  #ifdef DEBUG
  for (std::unordered_map<TomoCount*, unsigned int>::iterator it = deleteChecker.begin(); it != deleteChecker.end(); ++it)
  {
    TomoCount* tc = (*it).first;
    unsigned int val = (*it).second;
    
    tp.printTomonoid(tc->tomo);
    std::cerr << "We have value of " << val << std::endl;
  }
  #endif

  fs.close();
  end = std::chrono::system_clock::now();
  // std::cout << "Found that many tomonoids: " << cnt << std::endl;
  //delete t7;
  
  for (std::map<unsigned int, unsigned int>::iterator it = levelTomos.begin(); it != levelTomos.end(); ++it)
  {
    std::cout << "In level " << (*it).first << " there are " << (*it).second << " tomonoids" << std::endl;
  }
  
  //std::cout << "Created: " << created << ", deleted: " << deleted << ", finalized: " << finalized << std::endl;
  
  std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
 
    std::cerr << "finished computation at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s\n";

  return 0;
}

void printHelp()
{
  static const std::string help = "Tomonoid generator.\nARGUMENTS LIST (all are optional):";
  const unsigned int width = 15;
  std::cerr << help << std::endl;
  std::cerr << std::setw(width) << std::left << "-a" << "Generate only archimedean monoids." << std::endl;
  std::cerr << std::setw(width) << "-max [NUM]" 
  << "Maximum levels of depth search (How many new elements to add at lowest level)." << std::endl;
  std::cerr << std::setw(width) << "-o [FILENAME]" << "Name of output file (if not specified, generic file is created)." << std::endl;
  std::cerr << std::setw(width) << "-i [FILENAME]" << "Name of input file (must be succeeded by -id)." << std::endl;
  std::cerr << std::setw(width) << "-id [ID]" << "ID of root tomonoid for current session." << std::endl;
  std::cerr << std::setw(width) << "-multi" << "Enable multi-threading." << std::endl;
  std::cerr << std::setw(width) << "-optsave" << "Optimize saving (produces lesser output files, but execution may take more time)." << std::endl;
}

void readArgs(int argc, char** argv)
{
  for (int i = 1; i < argc; i++) // first argument is name of program
  {
    std::string command(argv[i]);
    if (command == "-a") // generate only Archimedean t.
    {
	onlyArchimedean = true;
	std::cerr << "Only archimedean tomonoids will be generated." << std::endl;
    } 
    else if (command =="-c") // generate only commutative t.
    {
	onlyCommutative = true;
	// When only commutative option will work...
	//std::cerr << "Only commutative tomonoids will be generated." << std::endl;
    }
    else if (command =="-max") // Maximum depth
    {
	i++;
	if (i >= argc)
	{
	  throw std::invalid_argument("Maximum level (-max) argument must be succeeded by positive integer.");
	}
	std::string num(argv[i]);
	try {
	  maxLevels = std::stoi(num);
	} 
	catch (std::invalid_argument)
	{
	  throw std::invalid_argument("Maximum level (-max) argument must be succeeded by positive integer, found \"" + num + "\".");
	}
	if (maxLevels < 1)
	{
	  throw std::invalid_argument("Maximum level (-max) must be greater than 0.");
	}
	std::cerr << "Maximum level: " << num << std::endl;
    }
    else if (command == "-o") // Output file
    {
	i++;
	if (i >= argc)
	{
	  throw std::invalid_argument("Unspecified output file name.");
	}
	std::string file(argv[i]);
	outputName = file;
	std::cerr << "Output file: " << file << std::endl;
    }
    else if (command == "-i") // Input file
    {
	i++;
	// Get input file name.
	if (i >= argc) // if we reach end, then it is an error (unspecified name).
	{
	  throw std::invalid_argument("Unspecified input file name.");
	}
	input_name = std::string(argv[i]);
	
	std::cerr << "Input file: " << input_name << std::endl;
	
	i++;
	if (i >= argc)
	{
	  continue; // End reached, but next arg is just optional so no need for exception.
	}
	std::string option(argv[i]);
	if (option == "-id") // Generate descendants of Tomonoid with given ID.
	{
	  input_type_levels = false;
	  input_max_levels = false;
	  i++;
	  if (i >= argc)
	  {
	    throw std::invalid_argument("ID unspecified.");
	  }
	  std::string id_str(argv[i]);
	  try {
	    input_type_arg = std::stoi(id_str);
	  } 
	  catch (std::invalid_argument)
	  {
	    throw std::invalid_argument("Tomonoid ID must be an integer, found \"" + id_str + "\".");
	  }
	  std::cerr << "Base tomonoid ID: " << id_str << std::endl;
	  input = true;
	}
	else if (option == "-l") // starting level/size of tomonoid
	{
	  i++;
	  input_max_levels = false;
	  if (i >= argc)
	  {
	    throw std::invalid_argument("Level unspecified.");
	  }
	  std::string id_str(argv[i]);
	  try {
	    input_type_arg = std::stoi(id_str);
	  } 
	  catch (std::invalid_argument)
	  {
	    throw std::invalid_argument("Starting level must be an integer, found \"" + id_str + "\".");
	  }
	  if (maxLevels < 1)
	  {
	    throw std::invalid_argument("Starting level must be greater than 0");
	  }
	  std::cerr << "Base level: " << id_str << std::endl;  
	}
	else // Probably next option, so act like if nothing happened and return to the loop.
	{
	  i--;
	}
    }
    else if (command == "-multi") // Enable multi-threading
    {
      multi = true;
      std::cerr << "Multi-threading enabled." << std::endl;
    }
    else if (command == "-optsave")
    {
      optimizingSaveMode = true;
      std::cerr << "Saving optimization enabled." << std::endl;
    }
    else if (command == "-h" || command == "--help")
    {
      printHelp();
      help = true;
    }
    else // Unknown option
    {
      std::string unk_option(argv[i]);
      std::cerr << "Unknown or wrongly placed option " << unk_option << " will be ignored. See -h or --help." << std::endl;
    }
  }
}

void TomonoidPrinter::printTomonoid(Tomonoid* tomo)
{
    static int now_printing = 1;
    int size = tomo->getSize();
    int width = (int) log10(size - 2) + 2;
    std::cout << "Printing Tomonoid no. " << now_printing++ << ", address: " << tomo << " of size " << size << std::endl;
    for (int i = 0; i < size - 1; i++)
    {
      std::cout << std::setw(width) << std::right << i;
    }
    std::cout << std::setw(width) << std::right << "T" << std::endl;
    std::cout << std::setfill('-') << std::setw(width * (size+1)) << "|" << std::endl;
    
    for (int i = 0; i < size - 1; i++)
    {
      std::cout << std::setw(width) << std::right << i;
    }
    std::cout << std::setw(width) << std::right << "T" << " | T" << std::endl;
    
    for (int i = size - 2; i > 0; i--) // po rade -> vpravo -> pravy clen
    {
      std::cout << std::setw(width) << std::right << "0";
      std::shared_ptr<const Element> el1 = ElementCreator::getInstance().getElementPtr(i, size); // pravy
      for (int j = 1; j < size - 1; j++)
      {
	
	std::shared_ptr<const Element> el2 = ElementCreator::getInstance().getElementPtr(j, size); // levy (sloupce)
	const Element result = tomo->getResult(el2, el1);
	if (result == Element::bottom_element) {
	  std::cout << std::setw(width) << std::right << "0";
	} else {
	  std::cout << std::setw(width) << std::right << (size - result.getOrder() - 1);
	}
	  
      }
      std::cout << std::setw(width) << std::right << i << " | " << i << std::endl;
    }
    
    for (int i = 0; i < size; i++)
    {
      std::cout << std::setw(width) << std::right << "0";
    }
    std::cout << " | 0" << std::endl;
    
}

// ASI THRASH
static const int vrcholy_count = 6;
static int hrany_odkaz[] = {0,-1,1,3,-1,-1};
static int sousedu[] = {1,0,2,1,0,0};
static int hrany[] = {2,3,5,5};
static bool zabarvena_komponenta[vrcholy_count];
static int forced_turn[vrcholy_count]; // aka kdy naposled byl vrchol forcovan do jednicky :)
static int zdroje[] = {0,-1};//{0,1,4,-1};
static int zdroje_len = 1;
static int print_order = 0;
  
void print_it()
{
  std::cout << print_order << ": ";
    for (int i = 0; i < vrcholy_count; i++)
    {
      std::cout << zabarvena_komponenta[i];
    }
    std::cout << std::endl;
    print_order++;
}

void permute_zdroje(int zdroj, int vrchol, bool force)
{
  if (zdroj == zdroje_len)
  {
    print_it();
  }
  else
  {
    if (vrchol == zdroje[zdroj])
    {
      permute_zdroje(zdroj + 1, zdroje[zdroj + 1], false);
      
      int odkaz_curr = hrany_odkaz[vrchol];
      int ssedu = sousedu[vrchol];
      for (int i = odkaz_curr; i < odkaz_curr + ssedu; i++)
      {
	permute_zdroje(zdroj, hrany[i], false);
      }
      
      zabarvena_komponenta[vrchol] = true;
      
      for (int i = odkaz_curr; i < odkaz_curr + ssedu; i++)
      {
	permute_zdroje(zdroj, hrany[i], true);
      }
      // po forcu printuju tady, protoze musim pockat, az se to vse zpropaguje
      print_it();
      
      permute_zdroje(zdroj + 1, zdroje[zdroj + 1], false);
    }
    else // jsme v nizsich oblastech
    {
      if (force && forced_turn[vrchol] != print_order)
      {
	// odpropaguju niz a nic nedelam
	zabarvena_komponenta[vrchol] = true;
	forced_turn[vrchol] = print_order;
	
	int odkaz_curr = hrany_odkaz[vrchol];
	int ssedu = sousedu[vrchol];
	for (int i = odkaz_curr; i < odkaz_curr + ssedu; i++)
	{
	  permute_zdroje(zdroj, hrany[i], true);
	}
      }
      else if (!force)
      {
	
      }
    }
  }
}

void check_graph_perm()
{
  for (int i = 0; i < vrcholy_count; i++)
  {
    zabarvena_komponenta[i] = false;
  }
  int start_vrchol = zdroje[0];
  permute_zdroje(0, start_vrchol, false);
}

bool assocTest(Tomonoid *t)
{
  int sz = t->getSize();
  bool ret= true;
  for (int a = 1; a < sz - 1; a++)
  {
      std::shared_ptr<const Element> aPtr = ElementCreator::getInstance().getElementPtr(a, sz);
      for (int b = 1; b < sz - 1; b++)
      {
	std::shared_ptr<const Element> bPtr = ElementCreator::getInstance().getElementPtr(b, sz);
	TableElement ab(aPtr, bPtr);
	const Element& abRes = t->getResult(ab);
	
	if (abRes == Element::bottom_element)
	{
	  continue;
	}
	
	std::shared_ptr<const Element> dResPtr = ElementCreator::getInstance().getElementPtr(abRes);
	
	for (int c = 1; c < sz - 1; c++)
	{
	  std::shared_ptr<const Element> cPtr = ElementCreator::getInstance().getElementPtr(c, sz);
	  TableElement bc(bPtr, cPtr);
	  const Element& bcRes = t->getResult(bc);
	  if (bcRes == Element::bottom_element)
	  {
	    continue;
	  }
	  
	  std::shared_ptr<const Element> eResPtr = ElementCreator::getInstance().getElementPtr(bcRes);
	  TableElement ae(aPtr, eResPtr);
	  TableElement dc(dResPtr, cPtr);
	  
	  const Element& aeRes = t->getResult(ae);
	  const Element& dcRes = t->getResult(dc);
	  
	  if (aeRes != dcRes)
	  {
	    std::cout << "Associativity fail for a = " << *(aPtr.get()) << " b = " << *(bPtr.get());
	    std::cout << " c = " << *(cPtr.get()) << std::endl;
	    std::cout << " ae = " << aeRes << ", dc = " << dcRes << std::endl;
	    ret = false;
	  }
	}
      }
  }
  return ret;
}
  
Tomonoid* createt9Tomo()
{

  Tomonoid *t7 = new Tomonoid(9);
  std::shared_ptr<const Element> z = ElementCreator::getInstance().getElementPtr(7, *t7);
  std::shared_ptr<const Element> y = ElementCreator::getInstance().getElementPtr(6, *t7);
  std::shared_ptr<const Element> x = ElementCreator::getInstance().getElementPtr(5, *t7);
  std::shared_ptr<const Element> w = ElementCreator::getInstance().getElementPtr(4, *t7);
  std::shared_ptr<const Element> v = ElementCreator::getInstance().getElementPtr(3, *t7);
  std::shared_ptr<const Element> u = ElementCreator::getInstance().getElementPtr(2, *t7);
  std::shared_ptr<const Element> t = ElementCreator::getInstance().getElementPtr(1, *t7);
  //std::vector<std::vector<Tomonoid*>> vecOfVecOfTomos;

  std::unordered_map<TableElement, std::shared_ptr<const Element>> resmap;
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(v,z), v));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(v,y), v));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(y,x), v));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(y,w), v));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(y,v), v));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(z,v), v));
  
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(w,z), w));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(w,y), w));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(x,y), w));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(z,w), w));
  
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(x,z), x));
  resmap.insert(std::pair<TableElement, std::shared_ptr<const Element>>(TableElement(z,x), x));
  t7->setImportantResults(resmap);
  
  std::vector<std::shared_ptr<const Element>> nonarchs;
  nonarchs.push_back(z);
  nonarchs.push_back(y);
  t7->setNonarchimedeanArray(nonarchs);
  
  TomonoidPrinter tp;
  tp.printTomonoid(t7);
  return t7;
  
}

Tomonoid* create7tomo()
{

  Tomonoid *t2 = new Tomonoid(7);
  
  std::unordered_map<TableElement, std::shared_ptr<const Element>> ir;
  std::shared_ptr<const Element> z = ElementCreator::getInstance().getElementPtr(5, *t2);
  std::shared_ptr<const Element> y = ElementCreator::getInstance().getElementPtr(4, *t2);
  std::shared_ptr<const Element> x = ElementCreator::getInstance().getElementPtr(3, *t2);
  std::shared_ptr<const Element> w = ElementCreator::getInstance().getElementPtr(2, *t2);
  std::shared_ptr<const Element> v = ElementCreator::getInstance().getElementPtr(1, *t2);
  
  TableElement te(x,z);
  ir.insert(std::make_pair(te, v));
  TableElement te2(y,z);
  ir.insert(std::make_pair(te2, v));
  TableElement te3(z,z);
  ir.insert(std::make_pair(te3, w));
  TableElement te4(z,y);
  ir.insert(std::make_pair(te4, v));
  
  t2->setImportantResults(ir);
  TomonoidPrinter tp;
  tp.printTomonoid(t2);
  
  return t2;
  //Tomonoid *t1 = new Tomonoid();
  
}

void setOutputName()
{
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  
  //TODO co kdyz slozka output neexistuje?
  std::ostringstream oss;
  oss << "output/";
  oss << std::put_time(&tm, "%d_%m_%Y_%H%M%S") << ".txt";
  outputName = oss.str();
}

