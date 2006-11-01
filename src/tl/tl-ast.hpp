#ifndef TL_AST_HPP
#define TL_AST_HPP

#include <string>
#include <set>
#include <map>
#include <vector>
#include "cxx-ast.h"
#include "cxx-prettyprint.h"
#include "tl-object.hpp"
#include "tl-predicate.hpp"

namespace TL
{
	class ScopeLink;


	class AST_t : public Object
	{
		private:
			AST _ast;
			static void tree_iterator(const AST_t& a, const Predicate& p, std::vector<AST_t>& result);
			tl_type_t* get_extended_attribute(const std::string& name) const;
		public:
			/*
			 * Constructor
			 */
			AST_t(AST _wrapped_tree)
				: _ast(_wrapped_tree)
			{
			}

            AST_t(Object obj)
            {
                AST_t* cast = dynamic_cast<AST_t*>(&obj);
                this->_ast = cast->_ast;
            }

			// AST_t(AST_t& ast)
			// 	: _ast(ast._ast)
			// {
			// }

			static AST get_translation_unit(AST node);
			static void prepend_list(AST orig_list, AST prepended_list);
			static void append_list(AST orig_list, AST appended_list);
			static void relink_parent(AST previous_child, AST new_child);

			static bool is_extensible_block(AST node);
			static AST get_list_of_extensible_block(AST node);

			/*
			 * Destructor
			 */
			virtual ~AST_t()
			{
			}

			bool operator<(AST_t n);
			bool operator==(AST_t n);
			AST_t& operator=(AST_t n);

			std::string prettyprint() const;

			void replace_with(AST_t ast);

			AST_t duplicate() const;

			std::vector<AST_t> get_all_subtrees_predicate(const Predicate& p) const;

			std::string internal_ast_type() const;

			virtual bool is_ast() const
			{
				return true;
			}

			void prepend_to_translation_unit(AST_t t);
			void append_to_translation_unit(AST_t t);

			void append(AST_t t);
			void prepend(AST_t t);

			AST_t get_enclosing_block();
			AST_t get_enclosing_function_definition();

			friend class Type;
            friend class Scope;
            friend class ScopeLink;
            friend class DepthTraverse;
	};

    typedef std::vector<AST_t> AST_list_t;
    typedef std::vector<AST_t> AST_set_t;
}

#endif // TL_AST_HPP
