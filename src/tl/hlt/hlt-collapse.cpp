#include "hlt-collapse.hpp"

using namespace TL::HLT;

LoopCollapse::LoopCollapse(TL::ForStatement for_stmt)
    : _for_nest_info(for_stmt)
{
    bool valid = true;
    if (!_for_nest_info.is_perfect())
    {
        valid = false;
        _ostream
            << "loop nest is not perfect" << std::endl;
    }
    if (valid && !_for_nest_info.is_all_regular())
    {
        valid = false;
        _ostream
            << "all loops of the loop nest must be regular" << std::endl;
    }
    if (!valid)
    {
        set_identity(for_stmt.get_ast());
    }
}

TL::Source LoopCollapse::get_source()
{
    ObjectList<ForStatement> for_nest_list = _for_nest_info.get_nest_list();
    // Fast path that it is not actually an error

    if (for_nest_list.size() == 1)
    {
        return for_nest_list[0].prettyprint();
    }

    Source collapsed_loop, header, collapsed_for;

    collapsed_loop
        << "{"
        << header
        << collapsed_for
        << "}"
        ;

    // We need to compute several things here
    // iteration_count_XXX is the number of iterations that every loop does
    // total_iteration_count is the total number of iterations
    // FIXME - Use a proper type not just 'int'

    Source total_iters; 
    Source iter_count_list; 

    header
        << iter_count_list
        << total_iters
        ;

    Source total_iters_init;
    total_iters
        << "const int _total_iters = " << total_iters_init << ";"
        ;

    int loop_n = 0;
    for (ObjectList<ForStatement>::iterator it = for_nest_list.begin();
            it != for_nest_list.end();
            it++, loop_n++)
    {
        // Iteration count
        {
            Source var_name;
            var_name << "_iteration_count_" << loop_n;

            iter_count_list
                << "const int " << var_name << "= (((" << it->get_upper_bound() << ")-(" << it->get_lower_bound() << ") + 1)"
                 "+(" << it->get_step() << ") - 1) / (" << it->get_step() << ");"
                ;
            total_iters_init.append_with_separator(var_name , "*");
        }

        // Lower
        {
            Source var_name;
            var_name << "_lower_" << loop_n;

            iter_count_list
                << "const int " << var_name << " = " << it->get_lower_bound() << ";"
                ;
        }

        // Step
        {
            Source var_name;
            var_name << "_step_" << loop_n;

            iter_count_list
                << "const int " << var_name << " = " << it->get_step() << ";"
                ;
        }
    }

    loop_n = 0;
    for (ObjectList<ForStatement>::iterator it = for_nest_list.begin();
            it != for_nest_list.end();
            it++, loop_n++)
    {
        // _current_count_0 is not used at all cause it is the same as _total_iters
        if (loop_n != 0)
        {
            // See a long comment below to know what is current_count[i]
            Source var_name;
            var_name << "_current_count_" << loop_n;

            Source current_count_value;

            header << "const int " << var_name << " = " << current_count_value << ";"
                ;

            for (int i = loop_n; i < for_nest_list.size(); i++)
            {
                current_count_value.append_with_separator(Source("_iteration_count_") << i, "*");
            }
        }
    }

    Source compute_nested_indexes, original_loop_body;
    // FIXME - Use a proper type, not just _m
    collapsed_for
        // FIXME - We may want to change this name _m
        << "int _m;"
        << "for(_m = 0; _m < _total_iters; _m++)"
        << "{"
        <<  compute_nested_indexes
        <<  original_loop_body
        << "}"
        ;

    loop_n = 0;
    for (ObjectList<ForStatement>::iterator it = for_nest_list.begin();
            it != for_nest_list.end();
            it++, loop_n++)
    {
        // Computing the proper index is done using the following formula
        //
        // indvar[i] = ((m % prod{j=i..N-1, count[j]}) / prod{j=i+1..N-1, count[j]}) * step[i] + lower[i]

        // where
        // - loops are identified from 0 to N-1, and each induction variable is
        // indvar[i], each step is step[i], each total iteration count (defined
        // as the number of iterations the loop performs) is count[i], each lower bound
        // is lower[i]
        // - prod{i=e0..e1, expr(i)} means the product of sequence expr(i) with
        // varying i from e0 to e1 (both included). If this range is such that e1 < e0
        // then the whole expression is 1
        //
        // Obvious simplifications
        //    indvar[0] = (m / prod{j=i+1..N-1, count[j]}) * step[0] + lower[0]
        //    indvar[N-1] = (m % count[N-1]) * step[N-1] + lower[N-1]
        //
        // For efficiency reasons we simplify the above formulae to
        //
        // indvar[i] = ((m % current_count[i]) / current_count[i + 1]) * step[i] + lower[i]
        //
        // indvar[0] = (m / current_count[1]) * step[0] + lower[0]
        // indvar[N-1] = (m % count[N-1]) * step[N-1] + lower[N-1]

        // where
        // - current_count[i] is prod{j=i..N-1, count[j]}

        ForStatement &for_stmt(*it);

        Symbol induction_var_symbol = for_stmt.get_induction_variable().get_symbol();
        Type induction_var_type = induction_var_symbol.get_type();

        Source initialization;

        compute_nested_indexes
            << induction_var_type.get_declaration(induction_var_symbol.get_scope(),
                    induction_var_symbol.get_name()) << " = " << initialization << ";";

        if (loop_n == 0)
        {
            initialization
                << "(_m / _current_count_1) * _step_0 + _lower_0"
                ;
        }
        else if (loop_n == (for_nest_list.size() - 1))
        {
            initialization
                << "(_m % _iteration_count_" << loop_n << ") * _step_" << loop_n << " + _lower_" << loop_n
                ;
        }
        else 
        {
            initialization
                << "((_m % _current_count_" << loop_n << ") / _current_count_" << (int)(loop_n + 1) << ") * _step_" << loop_n 
                << " + " << "_lower_" << loop_n
                ;
        }
    }

    original_loop_body
        << for_nest_list[for_nest_list.size() - 1].get_loop_body()
        ;

    return collapsed_loop;
}

LoopCollapse TL::HLT::loop_collapse(ForStatement for_stmt)
{
    return LoopCollapse(for_stmt);
}
