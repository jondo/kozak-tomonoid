#include "tomonoid.h"

// Remember, order is reverse (lower order -> greater element).
Element::Element(const unsigned int order, ElementType type)
{
  this->order = order;
  this->type = type;
}

Element::Element(const Element& other)
{
  this->order = other.order;
  this->type = other.type;
}
  
Element::~Element() 
{
  //std::cout << *this << " is being deleted." << std::endl;
  
}
  
bool operator==(const Element& e1, const Element& e2)
{
  return e1.type == e2.type && e1.order == e2.order;
}

bool operator!=(const Element& e1, const Element& e2)
{
  return !(e1 == e2);
}
  
bool operator<(const Element& e1, const Element& e2)
{
  // order is reverse!
  return e1.type < e2.type || (e1.type == e2.type && e1.order > e2.order);
}

bool operator>=(const Element& e1, const Element& e2)
{
  return !operator<(e1, e2);
}

bool operator>(const Element& e1, const Element& e2)
{
  return operator<(e2, e1);
}

bool operator<=(const Element& e1, const Element& e2)
{
  return !operator>(e1, e2);
}
  
std::string Element::toString() const
{
  return "Element: {type: " + std::to_string(type) + ", order: " + std::to_string(order) + "}";
}

std::ostream& operator<<(std::ostream& stream, const Element& el)
{
  stream << "Element: {type: " << el.type << ", order: " << el.order << "}";
  return stream;
}

const Element Element::bottom_element(0, BOTTOM);
const Element Element::top_element(0, TOP);

Element& Element::operator=(Element& other)
{
  // equality check not needed as assignment is cheap
  /*if (this == &other)
  {
    return *this;
  }
  else 
  {*/
    order = other.order;
    type = other.type;
    return *this;
  //}  
}