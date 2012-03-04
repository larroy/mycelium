/*
 * Copyright 2007 Pedro Larroy Tovar
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

#include "Url_classifier.hh"

using namespace std;

bool Url_classifier::empty()
{
    if( ! top_q.empty() )
        return false;

    for(tbl_seq_idx_t::iterator i = table.get<seq>().begin(); i != table.get<seq>().end(); ++i)
        if( ! i->queue->empty())
            return false;

    return true;
}

size_t Url_classifier::q_len(size_t num)
{
    tbl_n_idx_t::iterator i;
    if( (i = table.get<n>().find(num)) != table.get<n>().end() ) {
        return i->queue->size();
    } else
        throw runtime_error("no such num exist");
}

size_t Url_classifier::q_len_top()
{
    return top_q.size();
}


bool Url_classifier::empty(size_t num)
{
    tbl_n_idx_t::iterator i;
    if( (i = table.get<n>().find(num)) != table.get<n>().end() ) {
        if( i->queue->empty() )
            return true;
        else
            return false;
    } else
        throw runtime_error("no such num exist");
}

bool Url_classifier::empty_top()
{
    if( ! top_q.empty() )
        return false;
    else
        return true;
}


void Url_classifier::push(const Url& u)
{
    tbl_host_idx_t::iterator i;
    if( (i = table.get<host>().find(u.host())) != table.get<host>().end() ) {
        // an elemt with this h exists, put it in that q
        //cout << "push in q: " << i->n << endl;
        i->queue->push_back(u);
    } else {
        for(tbl_seq_idx_t::iterator j = table.get<seq>().begin(); j != table.get<seq>().end(); ++j) {
            if( j->queue->empty()) {
                table_elmt_t t(*j);
                t.host = u.host();
                t.queue->push_back(u);
                assert(table.get<seq>().replace(j,t) == true);
                return;
            }
        }

        // put it in that top_q
        //cout << "push in top_q" << endl;
        top_q.push(u);
    }
}

ostream& operator<<(ostream& os, const Url_classifier& u)
{
//    std::priority_queue<Url, std::deque<Url> >::iterator ti;
//    os << "top_q: " << endl;
//    for( ti = top_q.begin(); ti != top_q.end(); ++ti) {
//        os << "\t" << (*ti).get() << endl;
//    }

    Url_classifier::tbl_n_idx_t::iterator i;
    std::deque<Url>::iterator qi;
    os << "-------------" << endl;
    os << "Classifier dump:" << endl;
    for( i = u.table.get<Url_classifier::n>().begin(); i != u.table.get<Url_classifier::n>().end(); ++i) {
        os << "queue " << i->n << ":" << endl;
        for( qi = i->queue->begin(); qi != i->queue->end(); ++qi) {
            os << "\t" << qi->get() << endl;
        }
    }
    os << "-------------" << endl;
    return os;
}

//static bool less_for_uinfo(const Url& left, const Url& right)
////static bool Url_classifier::less_for_uinfo(const Url& left, const Url& right)
//{
//    return left.host() < right.host();
//
//}

bool Less_url::operator()(const Url& left, const Url& right)
//static bool Url_classifier::less_for_uinfo(const Url& left, const Url& right)
{
    return left.host() < right.host();

}


void Url_classifier::pop(size_t num)
{
    tbl_n_idx_t::iterator i;
    if( (i = table.get<n>().find(num)) != table.get<n>().end() ) {
        if( i->queue->empty() && top_q.empty() )
            throw runtime_error("empty");
        else if( i->queue->empty() ) {
            throw runtime_error("empty classifying queue");
        } else {
            i->queue->pop_front();
        }
    } else {
        throw runtime_error("No such queue");
    }
}


Url& Url_classifier::peek(size_t num)
{
    tbl_n_idx_t::iterator i;
    if( (i = table.get<n>().find(num)) != table.get<n>().end() ) {
        if( i->queue->empty() && top_q.empty() )
            throw runtime_error("empty");
        else if( i->queue->empty() ) {
            //cout << "deq from top_q" << endl;
            table_elmt_t t(*i);
            t.host = top_q.top().host();
            while( ! top_q.empty() && top_q.top().host() == t.host ) {
                t.queue->push_back(top_q.top());
                top_q.pop();
            }
            assert(table.get<n>().replace(i,t) == true);
            return t.queue->front();
        } else {
            return i->queue->front();
        }
    } else if(! top_q.empty()) {
        //cout << "new q n: " << num << endl;
        table_elmt_t t(num);


        t.host = top_q.top().host();
        while( ! top_q.empty() && top_q.top().host() == t.host ) {
            t.queue->push_back(top_q.top());
            top_q.pop();
        }
        //assert(table.get<n>().replace(i,t) == true);
        pair<tbl_n_idx_t::iterator,bool> p = table.get<n>().insert(t);
        assert(p.second == true);
        (void) p;
        return t.queue->front();
    } else {
        throw runtime_error("empty");
    }
}

/* End of Url_classifier implementation */
/************************************/

