#include "tomonoid.h"
#include <cassert>

ElementCreator::ElementCreator()
{
  this->elementsArray = new elements_vector();
  for (int i = 0; i < ElementCreator::DEFAULT_SIZE; i++)
  {
    std::shared_ptr<const Element> el = std::make_shared<const Element>(i + 1, ORDINARY);
    elementsArray->push_back(el);
  }
  this->size = ElementCreator::DEFAULT_SIZE;
}

ElementCreator::~ElementCreator()
{
  delete elementsArray;
}
  
ElementCreator& ElementCreator::getInstance()
{
  static ElementCreator instance;
  return instance;
}
  
const Element& ElementCreator::getElement(const unsigned int position, const unsigned int tomonoidSize)
{
  assert(position >= 0 && position < tomonoidSize);
  if (position == 0)
  {
    return Element::bottom_element;
  }
  else if (position == tomonoidSize - 1)
  {
    return Element::top_element;
  }
  else
  {
    std::shared_ptr<const Element> el = elementsArray->at(tomonoidSize - position - 2);
    return *(el.get());
  }
}
  
const Element& ElementCreator::getElement(unsigned int position, const Tomonoid& tomonoid)
{
  return getElement(position, tomonoid.getSize());
}

void ElementCreator::enlarge()
{
  for (int i = this->size; i < ElementCreator::DEFAULT_SIZE + this->size; i++)
  {
    std::shared_ptr<Element> el = std::make_shared<Element>(i + 1, ORDINARY);
    elementsArray->push_back(el);
  }
  this->size += ElementCreator::DEFAULT_SIZE;
}

std::shared_ptr<const Element> ElementCreator::getElementPtr(const Element& el)
{
  assert(el != Element::bottom_element && el != Element::top_element);
  return elementsArray->at(el.getOrder() - 1);
}

std::shared_ptr<const Element> ElementCreator::getElementPtr(unsigned int position, unsigned int tomonoidSize)
{
  assert(position > 0 && position < tomonoidSize - 1);
  return elementsArray->at(tomonoidSize - position - 2);
}

std::shared_ptr<const Element> ElementCreator::getElementPtr(unsigned int position, const Tomonoid& tomonoid)
{
  return getElementPtr(position, tomonoid.getSize());
}