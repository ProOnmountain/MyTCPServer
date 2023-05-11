#include "Data.h"

Data::Data(){
    m_body = nullptr;
}

Data::~Data(){
    delete [] m_body;
}