/*
 * Copyright 2007 Pedro Larroy Tovar 
 *
 * This file is subject to the terms and conditions
 * defined in file 'LICENSE.txt', which is part of this source
 * code package.
 */

/**
 * @addtogroup crawler 
 * @{
 */


#ifndef Url_classifier_hh
#define Url_classifier_hh

#include <deque>
#include <queue>
#include <boost/shared_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/indexed_by.hpp>

#include "Url.hh"

namespace mi = boost::multi_index;

/// Comparison function for the priority queue
class Less_url {
public:
	bool operator()(const Url& left, const Url& right);
};



/**
 * @brief A classifier which queues up urls grouped by host in N queues
 * 
 * top_q is queue for elements yet not classified

 * <PRE>
 *               __ queue # 1  host X
 *  top_q       /
 *  -----------/___ queue # 2  host Y
 *  host       \\
 *  {!X,!Y,!Z}  \\__ queue # 3  host Z
 *
 *
 *
 * </PRE>
 */
class Url_classifier {

public:
	Url_classifier() :  top_q(), table() {
	}
    /// Constructor, @param N is the number of queues
	Url_classifier(size_t N) :  top_q(), table() {
		for(size_t i=0; i<N; ++i) {
			table_elmt_t t(i);
			table.insert(t);
		}
	}

    /// Adds an url to classify and enqueue
	void push(const Url&);

    /// Take a peek at queue n 
	Url& peek(size_t n);

    /// Pop queue # @param n
	void pop(size_t n);

    /// Returns true if queue n
	bool empty(size_t n);
	//boost::shared_ptr<std::deque<Url> > queue(size_t num);

    /// @return true if all the queues are empty
	bool empty();

    /// @return true if the top queue is empty, meaning that all the items have been classified
	bool empty_top();

    /// @return number of elements in queue n 
	size_t q_len(size_t n);

    /// @return number of elements in top queue
	size_t q_len_top();
		
private:

	/**
	 * Element that holds a subqueue, with same host
	 */
	struct table_elmt_t {
		table_elmt_t(size_t nn) : host(), queue(new std::deque<Url>()), n(nn)
		{} 

		std::string host;
		boost::shared_ptr<std::deque<Url> > queue;
		/**
		 * Index of subqueue
		 */
		size_t	n;
	};

	// tags for access
	struct host {};
	//struct q {};
	struct n {};
	struct seq {};

	typedef mi::multi_index_container<
		table_elmt_t,
		mi::indexed_by<
			mi::ordered_non_unique<mi::tag<host>, mi::member<table_elmt_t,std::string,&table_elmt_t::host> >,
			mi::sequenced<mi::tag<seq> >,	
			//ordered_non_unique<tag<q>,	 member<table_elmt_t,boost::shared_ptr<std::deque<U> >,&table_elmt_t::q> >,
			mi::ordered_unique<mi::tag<n>, mi::member<table_elmt_t,size_t,&table_elmt_t::n> >
		> 
	> table_t;

	typedef table_t::index<host>::type tbl_host_idx_t;
	typedef table_t::index<n>::type tbl_n_idx_t;
	typedef table_t::index<seq>::type tbl_seq_idx_t;


	//boost::shared_ptr<std::deque<Url> > top_q;
	std::priority_queue<Url, std::deque<Url>, Less_url> top_q;
	table_t table;

	friend std::ostream& operator<<(std::ostream& os, const Url_classifier& u);
};


#endif
/** @} */

