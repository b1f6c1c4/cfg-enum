#ifndef LOGIC_H
#define LOGIC_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>

/* Sort */

class Sort {
public:
  virtual ~Sort() {}

  virtual std::string to_string() const = 0;

  virtual std::vector<std::shared_ptr<Sort>> get_domain_as_function() const = 0;
  virtual std::shared_ptr<Sort> get_range_as_function() const = 0;
};

class BooleanSort : public Sort {
  std::string to_string() const override;
  std::vector<std::shared_ptr<Sort>> get_domain_as_function() const override;
  std::shared_ptr<Sort> get_range_as_function() const override;
};

class UninterpretedSort : public Sort {
public:
  std::string const name;

  UninterpretedSort(std::string const& name) : name(name) { }
  std::string to_string() const override;
  std::vector<std::shared_ptr<Sort>> get_domain_as_function() const override;
  std::shared_ptr<Sort> get_range_as_function() const override;
};

class FunctionSort : public Sort {
public:
  std::vector<std::shared_ptr<Sort>> const domain;
  std::shared_ptr<Sort> range;

  FunctionSort(
      std::vector<std::shared_ptr<Sort>> const& domain,
      std::shared_ptr<Sort> range)
      : domain(domain), range(range) { }
  std::string to_string() const override;
  std::vector<std::shared_ptr<Sort>> get_domain_as_function() const override;
  std::shared_ptr<Sort> get_range_as_function() const override;
};

/* VarDecl */

typedef uint32_t iden;
std::string iden_to_string(iden);
iden string_to_iden(std::string const&);

class VarDecl {
public:
  iden name;
  std::shared_ptr<Sort> sort;

  VarDecl(
      iden name,
      std::shared_ptr<Sort> sort)
      : name(name), sort(sort) { }

  std::string to_string() {
    return iden_to_string(name) + ": " + sort->to_string();
  }
};

/* Value */

struct ScopeState;

class Value {
public:
  virtual ~Value() {}

  virtual std::string to_string() const = 0;
  virtual std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const = 0;
  virtual std::shared_ptr<Value> negate() const = 0;

  virtual std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const = 0;
  virtual std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const = 0;

  // only call this after uniquify_vars
  virtual std::shared_ptr<Value> structurally_normalize_() const = 0;

  virtual std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const = 0;
  virtual void get_used_vars(std::set<iden>&) const = 0;

  std::shared_ptr<Value> structurally_normalize() const {
    return this->uniquify_vars({})->structurally_normalize_();
  }

  std::shared_ptr<Value> totally_normalize() const;

  virtual int kind_id() const = 0;
};

class Forall : public Value {
public:
  std::vector<VarDecl> const decls;
  std::shared_ptr<Value> const body;

  Forall(
      std::vector<VarDecl> const& decls,
      std::shared_ptr<Value> body)
      : decls(decls), body(body) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 1; }
};

class Exists : public Value {
public:
  std::vector<VarDecl> const decls;
  std::shared_ptr<Value> const body;

  Exists(
      std::vector<VarDecl> const& decls,
      std::shared_ptr<Value> body)
      : decls(decls), body(body) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 2; }
};

class Var : public Value {
public:
  iden const name;
  std::shared_ptr<Sort> sort;

  Var(
      iden name,
      std::shared_ptr<Sort> sort)
      : name(name), sort(sort) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 50; }
};

class Const : public Value {
public:
  iden const name;
  std::shared_ptr<Sort> sort;

  Const(
      iden name,
      std::shared_ptr<Sort> sort)
      : name(name), sort(sort) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 4; }
};

class Eq : public Value {
public:
  std::shared_ptr<Value> left;
  std::shared_ptr<Value> right;

  Eq(
    std::shared_ptr<Value> left,
    std::shared_ptr<Value> right)
    : left(left), right(right) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 100; }
};

class Not : public Value {
public:
  std::shared_ptr<Value> val;

  Not(
    std::shared_ptr<Value> val)
    : val(val) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 6; }
};

class Implies : public Value {
public:
  std::shared_ptr<Value> left;
  std::shared_ptr<Value> right;

  Implies(
    std::shared_ptr<Value> left,
    std::shared_ptr<Value> right)
    : left(left), right(right) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 7; }
};

class Apply : public Value {
public:
  std::shared_ptr<Value> func;
  std::vector<std::shared_ptr<Value>> args;

  Apply(
    std::shared_ptr<Value> func,
    std::vector<std::shared_ptr<Value>> const& args)
    : func(func), args(args) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 8; }
};

class And : public Value {
public:
  std::vector<std::shared_ptr<Value>> args;

  And(
    std::vector<std::shared_ptr<Value>> const& args)
    : args(args) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 9; }
};

class Or : public Value {
public:
  std::vector<std::shared_ptr<Value>> args;

  Or(
    std::vector<std::shared_ptr<Value>> const& args)
    : args(args) { }

  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;

  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;

  int kind_id() const override { return 10; }
};

class TemplateHole : public Value {
public:
  TemplateHole() { }
  std::string to_string() const override;
  std::shared_ptr<Value> subst(iden x, std::shared_ptr<Value> e) const override;
  std::shared_ptr<Value> negate() const override;
  std::shared_ptr<Value> uniquify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> indexify_vars(std::map<iden, iden> const&) const override;
  std::shared_ptr<Value> structurally_normalize_() const override;
  std::shared_ptr<Value> normalize_symmetries(ScopeState const& ss, std::set<iden> const& vars_used) const override;
  void get_used_vars(std::set<iden>&) const override;
  int kind_id() const override { return 11; }
};

/* Action */

class Action {
public:
  virtual ~Action() {}
};

class LocalAction : public Action {
public:
  std::vector<VarDecl> args;
  std::shared_ptr<Action> body;

  LocalAction(
    std::vector<VarDecl> const& args,
    std::shared_ptr<Action> body)
    : args(args), body(body) { }
};

class SequenceAction : public Action {
public:
  std::vector<std::shared_ptr<Action>> actions;

  SequenceAction(
    std::vector<std::shared_ptr<Action>> const& actions)
    : actions(actions) { }
};

class ChoiceAction : public Action {
public:
  std::vector<std::shared_ptr<Action>> actions;

  ChoiceAction(
    std::vector<std::shared_ptr<Action>> const& actions)
    : actions(actions) { }
};

class Assume : public Action {
public:
  std::shared_ptr<Value> body;

  Assume(
    std::shared_ptr<Value> body)
    : body(body) { }
};

class Assign : public Action {
public:
  std::shared_ptr<Value> left;
  std::shared_ptr<Value> right;

  Assign(
    std::shared_ptr<Value> left,
    std::shared_ptr<Value> right)
    : left(left), right(right) { }
};

class IfElse : public Action {
public:
  std::shared_ptr<Value> condition;
  std::shared_ptr<Action> then_body;
  std::shared_ptr<Action> else_body;

  IfElse(
    std::shared_ptr<Value> condition,
    std::shared_ptr<Action> then_body,
    std::shared_ptr<Action> else_body)
    : condition(condition), then_body(then_body), else_body(else_body) { }
};

class If : public Action {
public:
  std::shared_ptr<Value> condition;
  std::shared_ptr<Action> then_body;

  If(
    std::shared_ptr<Value> condition,
    std::shared_ptr<Action> then_body)
    : condition(condition), then_body(then_body) { }
};

/* Module */

class Module {
public:
  std::vector<std::string> sorts;
  std::vector<VarDecl> functions;
  std::vector<std::shared_ptr<Value>> axioms;
  std::vector<std::shared_ptr<Value>> inits;
  std::vector<std::shared_ptr<Value>> conjectures;
  std::vector<std::shared_ptr<Value>> templates;
  std::vector<std::shared_ptr<Action>> actions;

  Module(
    std::vector<std::string> const& sorts,
    std::vector<VarDecl> const& functions,
    std::vector<std::shared_ptr<Value>> const& axioms,
    std::vector<std::shared_ptr<Value>> const& inits,
    std::vector<std::shared_ptr<Value>> const& conjectures,
    std::vector<std::shared_ptr<Value>> const& templates,
    std::vector<std::shared_ptr<Action>> const& actions)
    : sorts(sorts),
      functions(functions),
      axioms(axioms),
      inits(inits),
      conjectures(conjectures),
      templates(templates),
      actions(actions) { }
};

std::shared_ptr<Module> parse_module(std::string const& src);

typedef std::shared_ptr<Value> value;
typedef std::shared_ptr<Sort> lsort;

inline value v_forall(std::vector<VarDecl> const& decls, value const& body) {
  return std::shared_ptr<Value>(new Forall(decls, body));
}
inline value v_exists(std::vector<VarDecl> const& decls, value const& body) {
  return std::shared_ptr<Value>(new Exists(decls, body));
}
inline value v_var(iden name, lsort sort) {
  return std::shared_ptr<Value>(new Var(name, sort));
}
inline value v_const(iden name, lsort sort) {
  return std::shared_ptr<Value>(new Const(name, sort));
}
inline value v_eq(value a, value b) {
  return std::shared_ptr<Value>(new Eq(a, b));
}
inline value v_not(value a) {
  if (Not* n = dynamic_cast<Not*>(a.get())) {
    return n->val;
  } else {
    return std::shared_ptr<Value>(new Not(a));
  }
}
inline value v_implies(value a, value b) {
  return std::shared_ptr<Value>(new Implies(a, b));
}
inline value v_apply(value func, std::vector<value> const& args) {
  return std::shared_ptr<Value>(new Apply(func, args));
}
inline value v_and(std::vector<value> const& args) {
  if (args.size() == 1) return args[0];
  return std::shared_ptr<Value>(new And(args));
}
inline value v_true() {
  return std::shared_ptr<Value>(new And({}));
}
inline value v_false() {
  return std::shared_ptr<Value>(new Or({}));
}
inline value v_or(std::vector<value> const& args) {
  if (args.size() == 1) return args[0];
  return std::shared_ptr<Value>(new Or(args));
}
inline value v_template_hole() {
  return std::shared_ptr<Value>(new TemplateHole());
}

inline lsort s_bool() {
  return std::shared_ptr<Sort>(new BooleanSort());
}
inline lsort s_uninterp(std::string name) {
  return std::shared_ptr<Sort>(new UninterpretedSort(name));
}
inline lsort s_fun(std::vector<lsort> inputs, lsort output) {
  return std::shared_ptr<Sort>(new FunctionSort(inputs, output));
}

bool sorts_eq(lsort s, lsort t);

#endif
