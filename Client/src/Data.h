#pragma once
struct Header{
    char m_sender[32];
    char m_receiver[32];
    int m_bodySize;
    int m_dataType;
};

struct Data{
    Header m_header;
    char* m_body;
    Data();
    ~Data();
    
};