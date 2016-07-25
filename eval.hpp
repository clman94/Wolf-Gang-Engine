#include <string>
#include <map>
#include <functional>
#include "utility.hpp"

class evaluator
{
	std::string expression;
	std::map<std::string, double> variables;
	std::map<std::string, std::function<double(double)>> functions;
	
	bool is_whitespace(char c)
	{
		return (
			c == ' '  ||
			c == '\t' ||
			c == '\n' ||
			c == '\r'
			);
	}

	bool is_letter(char c)
	{
		return (
			c >= 'a' &&
			c <= 'z'
			);
	}

	void skip_whitespace(std::string::iterator& iter)
	{
		while (iter != expression.end() && is_whitespace(*iter))
			++iter;
	}

	std::string read_name(std::string::iterator& iter)
	{
		std::string name;
		while (iter != expression.end() && is_letter(*iter))
		{
			name += *iter;
			++iter;
		}
		return std::move(name);
	}

	double factor(std::string::iterator& iter)
	{
		skip_whitespace(iter);

		// Default at one to avoid divid-by-zero error
		double val = 1;

		if (iter == expression.end())
			return val;
		
		if (*iter == '(')
			val = summands(++iter);

		else if (*iter == '-')
			val = -factor(++iter);

		// Function
		else if (*iter == '$')
		{
			auto &func = functions.find(read_name(++iter));
			if (func == functions.end())
				return 0;
			val = func->second(factor(iter));
		}

		// Variable
		else if (is_letter(*iter))
			val = variables[read_name(iter)];

		// Numeral
		else
			val = util::to_numeral<double>(expression, iter);

		if (iter == expression.end())
			return val;

		if (*iter == '*')
			val *= factor(++iter);
		else if (*iter == '/')
			val /= factor(++iter);
		else if (*iter == '^')
			val = std::pow(val, factor(++iter));

		return val;
	}

	double summands(std::string::iterator& iter)
	{
		double v1 = factor(iter);
		while (iter != expression.end())
		{
			skip_whitespace(iter);
			if (*iter == '+')
				v1 += factor(++iter);
			else if (*iter == '-')
				v1 -= factor(++iter);
			else if (*iter == ')')
				return (++iter, v1);
			else
				return v1;
		}
		return v1;
	}

public:
	void set_expression(const std::string& ex)
	{
		expression = ex;
	}

	void set_function(const std::string& name, std::function<double(double)> func)
	{
		functions[name] = func;
	}

	double& operator[](const std::string& name)
	{
		return variables[name];
	}

	double evaluate()
	{
		std::string::iterator iter = expression.begin();
		return summands(iter);
	}

	double evaluate(const std::string& ex)
	{
		set_expression(ex);
		return evaluate();
	}

	static double quick_evaluate(const std::string& ex)
	{
		evaluator eval;
		return eval.evaluate(ex);
	}
};