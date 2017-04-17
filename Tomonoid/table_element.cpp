#include "tomonoid.h"


static int id = 0;

TableElement::TableElement()
{
  this->left = NULL;
  this->right = NULL;
  hash_val = 0;
  te_id = id;
  id++;
  //std::cout << "TableElement id: " << te_id << std::endl;
}

TableElement::TableElement(std::shared_ptr<const Element> left, std::shared_ptr<const Element> right)
{
  this->left = left;
  this->right = right;
  te_id = id;
  id++;
  this->hash_val = (left.get()->getOrder() << 16) + right.get()->getOrder();
  //std::cout << "TableElement id: " << te_id << std::endl;
}

TableElement::TableElement(const TableElement& other)
{
  this->left = other.left;
  this->right = other.right;
  te_id = id;
  id++;
  this->hash_val = other.hash_val;
  //std::cout << "TableElement id: " << te_id << std::endl;
}

TableElement::~TableElement()
{
  //std::cout << "Deleting TableElement, left count is: " << this->left.use_count() << std::endl;
  if (this->left != NULL)
  {
    this->left.reset();
  }
  if (this->right != NULL)
  {
    this->right.reset();
  }
  //std::cout << "TableElement deleted " << te_id << std::endl;
}
  
bool operator==(const TableElement& te1, const TableElement& te2)
{
  return (*(te1.left) == *(te2.left) && *(te1.right) == *(te2.right));
}

bool operator!=(const TableElement& te1, const TableElement& te2)
{
  return !(te1 == te2);
}
  
bool operator<(const TableElement& te1, const TableElement& te2)
{
   // jsem na nizsim RADKU - prava hodnota >
  // nebo jsem na stejnym a leva je mensi!
  return (*(te1.right) > *(te2.right)) || (*(te1.right) == *(te2.right) && *(te1.left) < *(te2.left));
}

bool operator>=(const TableElement& te1, const TableElement& te2)
{
  return !operator<(te1, te2);
}

bool operator>(const TableElement& te1, const TableElement& te2)
{
  return operator<(te2, te1);
}

bool operator<=(const TableElement& te1, const TableElement& te2) 
{
  return !operator>(te1, te2);
}

std::ostream& operator<<(std::ostream& os, const TableElement& te)
{
  os << "TableElement: left: " << *(te.left);
  os << ", right: " << *(te.right);
  return os;
}

TableElement& TableElement::operator=(TableElement& other)
{
  if (this == &other)
  {
    return *this;
  }
  else
  {
    left = other.left;
    right = other.right;
    hash_val = other.hash_val;
    return *this;
  }
}

const size_t TableElement::getHash() const
{
  return this->hash_val;
}