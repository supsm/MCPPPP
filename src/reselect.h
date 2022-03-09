/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <algorithm>
#include <cassert>
#include <list>
#include <sstream>
#include <string>
#include <vector>

class reselect
{
private:

	// lines does not contain any tabs or indentation
	// list is more of a pain to iterate, but we may need random insert
	std::list<std::string> lines;
	std::list<int> indentation;

	// get number of tabs in indentation
	// @param prev  previous line
	// @param prev_indent  indentation of previous line
	// @param cur  line to get indentation for
	// @return number of indents for the current line
	static int get_indent(const std::string& prev, int prev_indent, const std::string& cur)
	{
		// increment indent if previous line ends with "then" or starts with "if", "elseif", "else", "let", or "in"
		if (prev.ends_with("then") || prev.starts_with("if") || prev.starts_with("else") || prev.starts_with("let") || prev.starts_with("in"))
		{
			prev_indent++;
		}
		// decrement indent if current line is "elseif", "else", "in", "end", or "then"
		if (cur.starts_with("else") || cur.starts_with("in") || cur.starts_with("end") || cur.starts_with("then"))
		{
			prev_indent--;
		}
		return std::max(0, prev_indent);
	}

public:

	reselect() {}

	// conversion from path to reselect object
	// @param str  string to add to reselect
	// @param quotes  whether to add quotes around the string (default true)
	explicit reselect(const std::string& str, const bool quotes = true)
	{
		// TODO: check for special characters and escape them? (e.g. quotes)
		if (quotes)
		{
			lines.push_back('\"' + str + '\"');
		}
		else
		{
			lines.push_back(str);
		}
		indentation.push_back(0);
	}

	// get string to be printed
	// @return string which is to be outputted to a file
	std::string get_string() const
	{
		std::stringstream ss;
		// pair of iterators, first is indentation, second is lines
		for (auto p_it = std::make_pair(indentation.begin(), lines.begin());
			p_it.first != indentation.end() && p_it.second != lines.end();
			p_it.first++, p_it.second++)
		{
			// output tabs, then actual line
			ss << std::string(*p_it.first, '\t')
				<< *p_it.second << std::endl;
		}
		return ss.str();
	}

	std::size_t size() const noexcept
	{
		return lines.size();
	}

	bool empty() const noexcept
	{
		return lines.empty();
	}

	// indent from first to last
	// @param first  position of first element to indent (inclusive)
	// @param last  position of last element to indent (exclusive)
	void indent(int first = -1, int last = -1)
	{
		assert(first >= 0);
		assert(first < last);
		assert(last < lines.size());

		if (first == -1 || last == -1)
		{
			first = 0;
			last = lines.size();
		}

		// set start iterator and end iterator
		auto start_it = indentation.begin(), end_it = indentation.begin();
		std::advance(start_it, first);
		std::advance(end_it, last);

		// increment tab values
		for (auto it = start_it; it != end_it; it++)
		{
			(*it)++;
		}
	}

	// insert line after pos
	// @param pos  position after which the line will be inserted
	// @param line  line to be inserted
	void insert(const int pos, const std::string& line)
	{
		auto it = lines.begin();
		auto it2 = indentation.begin();
		// pos + 1 because we want to insert after
		// std::list::insert inserts before
		std::advance(it, pos + 1);
		std::advance(it2, pos + 1);

		int cur_indent = 0;
		if (pos > 0)
		{
			auto temp_it = it;
			auto temp_it2 = it2;
			temp_it--;
			temp_it2--;
			cur_indent = get_indent(*temp_it, *temp_it2, line);
		}

		lines.insert(it, line);
		indentation.insert(it2, cur_indent);
	}

	// insert reselect object after pos
	// @param pos  position after which the object will be inserted
	// @param res  reselect object to add (indentation is ignored)
	void insert(const int pos, const reselect& res)
	{
		int offset = 0;
		for (const auto& line : res.lines)
		{
			insert(pos + offset, line);
			offset++;
		}
	}

	// add line at end
	// @param line  line to add
	void push_back(const std::string& line)
	{
		int cur_indent = 0;
		if (!lines.empty())
		{
			cur_indent = get_indent(lines.back(), indentation.back(), line);
		}

		lines.push_back(line);
		indentation.push_back(cur_indent);
	}

	// add reselect object to end
	// @param res  reselect object to add (indentation is ignored)
	void push_back(const reselect& res)
	{
		for (const auto& line : res.lines)
		{
			push_back(line);
		}
	}

	// add if statement
	// if conditions[0] then statements[0]
	// elseif conditions[1] then statements[1]
	// ...
	// else default_statement
	// end
	// @param conditions  list of conditions to check
	// @param statements  statements to execute if condition is true (size must be equal to conditions)
	// @param default_statement  statement to execute if all conditions are false (else), empty for no else statement
	void add_if(const std::vector<reselect>& conditions, const std::vector<reselect>& statements, const reselect& default_statement = reselect("default", false))
	{
		assert(conditions.size() == statements.size());
		if (conditions.empty())
		{
			return;
		}

		// first one is if, rest are elseif
		if (conditions.front().size() == 1)
		{
			// single line
			push_back("if " + conditions.front().lines.front() + " then");
		}
		else
		{
			// multi line
			push_back("if");
			push_back(conditions.front());
			push_back("then");
		}
		push_back(statements.front());

		for (int i = 1; i < conditions.size(); i++)
		{
			if (conditions[i].lines.size() == 1)
			{
				// single line
				push_back("elseif " + conditions[i].lines.front() + " then");
			}
			else
			{
				// multi line
				push_back("elseif");
				push_back(conditions[i]);
				push_back("then");
			}
			push_back(statements[i]);
		}

		if (!default_statement.lines.empty())
		{
			push_back("else");
			push_back(default_statement);
		}

		push_back("end");
	}

	// add if statement with string conditons
	// @param conditions  list of conditions to check
	// @param statements  statements to execute if condition is true (size must be equal to conditions)
	// @param default_statement  statement to execute if all conditions are false (else), empty for no else statement
	void add_if(const std::vector<std::string>& conditions, const std::vector<reselect>& statements, const reselect& default_statement = reselect("default", false))
	{
		std::vector<reselect> r_conditions(conditions.size());
		std::transform(conditions.begin(), conditions.end(), r_conditions.begin(), [](const std::string& s) -> reselect
			{
				return reselect(s, false); // conditions should not have quotes, statements should
			});
		add_if(r_conditions, statements, default_statement);
	}

	// add if statement with string conditions and string statements
	// @param conditions  list of conditions to check
	// @param statements  statements to execute if condition is true (size must be equal to conditions)
	// @param default_statement  statement to execute if all conditions are false (else), empty for no else statement
	void add_if(const std::vector<std::string>& conditions, const std::vector<std::string>& statements, const std::string& default_statement = "default")
	{
		std::vector<reselect> r_statements(statements.size());
		std::transform(statements.begin(), statements.end(), r_statements.begin(), [](const std::string& s) -> reselect
			{
				return static_cast<reselect>(s);
			});
		add_if(conditions, r_statements, static_cast<reselect>(default_statement));
	}

	// randomly select between statements, appends to current object
	// @param seed  seed for random number generator (usually entity name)
	// @param statements  list of statements to choose from
	// @param weights  list of weights, empty for equal distribution (optional, default empty)
	void add_random(const std::string& seed, std::vector<reselect> statements, const std::vector<int>& weights = std::vector<int>())
	{
		int weightsum = 0; // total sum of weights
		// last statement goes into else
		reselect default_statement = statements.back();
		statements.pop_back();
		if (weights.empty())
		{
			// random seletion without weights
			weightsum = statements.size();

			// functions for readability and performance
			push_back("let");
			push_back("rand_id := rand(" + seed + ", 1, " + std::to_string(weightsum) + ")");
			push_back("rand_is(val) := rand_id == val");
			push_back("in");

			std::vector<std::string> conditions;
			for (int i = 0; i < statements.size(); i++)
			{
				conditions.push_back("rand_is(" + std::to_string(i + 1) + ")");
			}
			add_if(conditions, statements, default_statement);

			// end the let
			push_back("end");
		}
		else
		{
			// weighted random selection
			assert(statements.size() + 1 == weights.size()); // we already removed the last element from statements
			for (const auto& i : weights)
			{
				weightsum += i;
			}

			// functions for convenience and readability
			push_back("let");
			push_back("rand_id := rand(" + seed + ", 1, " + std::to_string(weightsum) + ")");
			push_back("rand_in(from, to) := rand_id >= from and rand_id <= to");
			push_back("in");

			// actual random thingy
			std::vector<std::string> conditions;
			int cursum = 0; // current sum of weights
			for (int i = 0; i < weights.size() - 1; i++)
			{
				conditions.push_back("rand_in(" + std::to_string(cursum + 1) + ", " + std::to_string(cursum + weights[i]) + ")");
				cursum += weights[i];
			}
			add_if(conditions, statements, default_statement);

			// end the let
			push_back("end");
		}
	}

	// randomly select between string statements, appends to current object
	// @param seed  seed for random number generator (usually entity name)
	// @param statements  list of statements to choose from
	// @param weights  list of weights, empty for equal distribution (optional, default empty)
	void add_random(const std::string& seed, const std::vector<std::string>& statements, const std::vector<int>& weights = std::vector<int>())
	{
		std::vector<reselect> r_statements(statements.size());
		std::transform(statements.begin(), statements.end(), r_statements.begin(), [](const std::string& s) -> reselect
			{
				// empty texture = default
				if (s.empty())
				{
					return reselect("default", false);
				}
				else
				{
					return static_cast<reselect>(s);
				}
			});
		add_random(seed, r_statements, weights);
	}

	// construct condition by linking multiple conditions with and
	// @param conditions  vectors which contains the conditions
	// @return single condition containing all other conditions linked using and
	static reselect construct_and(std::vector<reselect> conditions)
	{
		reselect res;
		// don't add parentheses if there is only 1
		if (conditions.size() != 1)
		{
			for (int i = 0; i < conditions.size() - 1; i++)
			{
				// add parenthesis
				conditions[i].lines.front().insert(conditions[i].lines.front().begin(), '(');
				conditions[i].lines.back().push_back(')');

				res.push_back(conditions[i]);
				res.push_back("and");
			}
			conditions.back().lines.front().insert(conditions.back().lines.front().begin(), '(');
			conditions.back().lines.back().push_back(')');
		}
		res.push_back(conditions.back());
		return res;
	}

	// construct condition by linking multiple conditions with and
	// @param conditions  vector which contains the conditions
	// @return single condition containing all other conditions linked using and
	static reselect construct_and(const std::vector<std::string>& conditions)
	{
		reselect res;
		for (int i = 0; i < conditions.size() - 1; i++)
		{
			res.push_back(conditions[i] + " and");
		}
		res.push_back(conditions.back());
		return res;
	}

	// construct condition by linking multiple conditions with or
	// @param conditions  vectors which contains the conditions
	// @return single condition containing all other conditions linked using or
	static reselect construct_or(std::vector<reselect> conditions)
	{
		reselect res;
		// don't add parentheses if there is only 1
		if (conditions.size() != 1)
		{
			for (int i = 0; i < conditions.size() - 1; i++)
			{
				// add parenthesis
				conditions[i].lines.front().insert(conditions[i].lines.front().begin(), '(');
				conditions[i].lines.back().push_back(')');

				res.push_back(conditions[i]);
				res.push_back("or");
			}
			conditions.back().lines.front().insert(conditions.back().lines.front().begin(), '(');
			conditions.back().lines.back().push_back(')');
		}
		res.push_back(conditions.back());
		return res;
	}

	// construct condition by linking multiple conditions with or
	// @param conditions  vector which contains the conditions
	// @return single condition containing all other conditions linked using or
	static reselect construct_or(const std::vector<std::string>& conditions)
	{
		reselect res;
		for (int i = 0; i < conditions.size() - 1; i++)
		{
			res.push_back(conditions[i] + " or");
		}
		res.push_back(conditions.back());
		return res;
	}
};
