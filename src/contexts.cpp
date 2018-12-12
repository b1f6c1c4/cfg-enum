#include "contexts.h"

using namespace std;
using z3::context;
using z3::solver;
using z3::sort;
using z3::func_decl;
using z3::expr;

/*
 * BackgroundContext
 */

BackgroundContext::BackgroundContext(z3::context& ctx, std::shared_ptr<Module> module)
    : ctx(ctx),
      solver(ctx)
{
  for (string sort : module->sorts) {
    this->sorts.insert(make_pair(sort, ctx.uninterpreted_sort(sort.c_str())));
  }
}

z3::sort BackgroundContext::getUninterpretedSort(std::string name) {
  return sorts.find(name)->second;
}

z3::sort BackgroundContext::getSort(std::shared_ptr<Sort> sort) {
  Sort* s = sort.get();
  if (dynamic_cast<BooleanSort*>(s)) {
    return ctx.bool_sort();
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s)) {
    return getUninterpretedSort(usort->name);
  } else if (FunctionSort* fsort = dynamic_cast<FunctionSort*>(s)) {
    assert(false && "getSort does not support FunctionSort");
  } else {
    assert(false && "getSort does not support unknown sort");
  }
}

/*
 * ModelEmbedding
 */

shared_ptr<ModelEmbedding> ModelEmbedding::makeEmbedding(
    shared_ptr<BackgroundContext> ctx,
    shared_ptr<Module> module)
{
  unordered_map<string, func_decl> mapping;
  for (VarDecl decl : module->functions) {
    Sort* s = decl.sort.get();
    if (FunctionSort* fsort = dynamic_cast<FunctionSort*>(s)) {
      z3::sort_vector domain(ctx->ctx);
      for (std::shared_ptr<Sort> domain_sort : fsort->domain) {
        domain.push_back(ctx->getSort(domain_sort));
      }
      z3::sort range = ctx->getSort(fsort->range);
      mapping.insert(make_pair(decl.name, ctx->ctx.function(
          decl.name.c_str(), domain, range)));
    } else {
      mapping.insert(make_pair(decl.name, ctx->ctx.function(decl.name.c_str(), 0, 0,
          ctx->getSort(decl.sort))));
    }
  }

  return shared_ptr<ModelEmbedding>(new ModelEmbedding(ctx, mapping));
}

z3::func_decl ModelEmbedding::getFunc(string name) {
  auto iter = mapping.find(name);
  assert(iter != mapping.end());
  return iter->second;
}

z3::expr ModelEmbedding::value2expr(
    shared_ptr<Value> value)
{
  return value2expr(value, {}, {});
}


z3::expr ModelEmbedding::value2expr(
    shared_ptr<Value> value,
    std::unordered_map<std::string, z3::expr> const& consts)
{
  return value2expr(value, consts, {});
}

z3::expr ModelEmbedding::value2expr(
    shared_ptr<Value> v,
    std::unordered_map<std::string, z3::expr> const& consts,
    std::unordered_map<std::string, z3::expr> const& vars)
{
  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    z3::expr_vector vec_vars(ctx->ctx);
    std::unordered_map<std::string, z3::expr> new_vars = vars;
    for (VarDecl decl : value->decls) {
      expr var = ctx->ctx.constant(decl.name.c_str(), ctx->getSort(decl.sort));
      vec_vars.push_back(var);
      new_vars.insert(make_pair(decl.name, var));
    }
    return z3::forall(vec_vars, value2expr(value->body, consts, new_vars));
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    auto iter = vars.find(value->name);
    assert(iter != vars.end());
    return iter->second;
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    auto iter = consts.find(value->name);
    assert(iter != consts.end());
    return iter->second;
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    return value2expr(value->left, consts, vars) == value2expr(value->right, consts, vars);
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    return !value2expr(value->value, consts, vars);
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    return !value2expr(value->left, consts, vars) || value2expr(value->right, consts, vars);
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    z3::expr_vector args(ctx->ctx);
    for (shared_ptr<Value> arg : value->args) {
      args.push_back(value2expr(arg, consts, vars));
    }
    Const* func_value = dynamic_cast<Const*>(value->func.get());
    assert(func_value != NULL);
    return getFunc(func_value->name)(args);
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    z3::expr_vector args(ctx->ctx);
    for (shared_ptr<Value> arg : value->args) {
      args.push_back(value2expr(arg, consts, vars));
    }
    return mk_and(args);
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    z3::expr_vector args(ctx->ctx);
    for (shared_ptr<Value> arg : value->args) {
      args.push_back(value2expr(arg, consts, vars));
    }
    return mk_or(args);
  }
  else {
    assert(false && "value2expr does not support this case");
  }
}

/*
 * InductionContext
 */

struct ActionResult {
  std::shared_ptr<ModelEmbedding> e;
  expr constraint;

  ActionResult(
    std::shared_ptr<ModelEmbedding> e,
    expr constraint)
  : e(e), constraint(constraint) { }
};

ActionResult applyAction(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Action> action,
    unordered_map<string, expr> const& consts);

InductionContext::InductionContext(
    std::shared_ptr<BackgroundContext> ctx,
    std::shared_ptr<Module> module)
    : ctx(ctx)
{
  this->e1 = ModelEmbedding::makeEmbedding(ctx, module);

  shared_ptr<Action> action = shared_ptr<Action>(new ChoiceAction(module->actions));
  ActionResult res = applyAction(this->e1, action, {});
  this->e2 = res.e;

  // Add the relation between the two states
  ctx->solver.add(res.constraint);

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e1->value2expr(axiom, {}));
  }
}

ActionResult do_if_else(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Value> condition,
    shared_ptr<Action> then_body,
    shared_ptr<Action> else_body,
    unordered_map<string, z3::expr> const& consts)
{
  vector<shared_ptr<Action>> seq1;
  seq1.push_back(shared_ptr<Action>(new Assume(condition)));
  seq1.push_back(then_body);

  vector<shared_ptr<Action>> seq2;
  seq2.push_back(shared_ptr<Action>(new Assume(shared_ptr<Value>(new Not(condition)))));
  if (else_body.get() != nullptr) {
    seq2.push_back(else_body);
  }

  vector<shared_ptr<Action>> choices = {
    shared_ptr<Action>(new SequenceAction(seq1)),
    shared_ptr<Action>(new SequenceAction(seq2))
  };

  return applyAction(e, shared_ptr<Action>(new ChoiceAction(choices)), consts);
}

expr funcs_equal(z3::context& ctx, func_decl a, func_decl b) {
  z3::expr_vector args(ctx);
  for (int i = 0; i < a.arity(); i++) {
    z3::sort arg_sort = a.domain(i);
    args.push_back(ctx.constant("arg", arg_sort));
  }
  return z3::forall(args, a(args) == b(args));
}

ActionResult applyAction(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Action> a,
    unordered_map<string, expr> const& consts)
{
  shared_ptr<BackgroundContext> ctx = e->ctx;

  if (LocalAction* action = dynamic_cast<LocalAction*>(a.get())) {
    unordered_map<string, expr> new_consts(consts);
    for (VarDecl decl : action->args) {
      func_decl d = ctx->ctx.function(decl.name.c_str(), 0, 0, ctx->getSort(decl.sort));
      expr ex = d();
      new_consts.insert(make_pair(decl.name, ex));
    }
    return applyAction(e, action->body, new_consts);
  }
  else if (SequenceAction* action = dynamic_cast<SequenceAction*>(a.get())) {
    z3::expr_vector parts(ctx->ctx);
    for (shared_ptr<Action> sub_action : action->actions) {
      ActionResult res = applyAction(e, sub_action, consts);
      e = res.e;
      parts.push_back(res.constraint);
    }
    expr ex = z3::mk_and(parts);
    return ActionResult(e, z3::mk_and(parts));
  }
  else if (Assume* action = dynamic_cast<Assume*>(a.get())) {
    return ActionResult(e, e->value2expr(action->body, consts));
  }
  else if (If* action = dynamic_cast<If*>(a.get())) {
    return do_if_else(e, action->condition, action->then_body, nullptr, consts);
  }
  else if (IfElse* action = dynamic_cast<IfElse*>(a.get())) {
    return do_if_else(e, action->condition, action->then_body, action->else_body, consts);
  }
  else if (ChoiceAction* action = dynamic_cast<ChoiceAction*>(a.get())) {
    int len = action->actions.size();
    vector<shared_ptr<ModelEmbedding>> es;
    z3::expr_vector parts(ctx->ctx);
    for (int i = 0; i < len; i++) {
      ActionResult res = applyAction(e, action->actions[i], consts);
      es.push_back(res.e);
      parts.push_back(res.constraint);
    }
    for (auto p : e->mapping) {
      string func_name = p.first;

      bool is_ident = true;
      func_decl new_func_decl = e->getFunc(func_name);
      for (int i = 0; i < len; i++) {
        if (es[i]->getFunc(func_name).name() != e->getFunc(func_name).name()) {
          is_ident = false;
          new_func_decl = es[i]->mapping.find(func_name)->second;
          break;
        }
      }
      if (!is_ident) {
        for (int i = 0; i < len; i++) {
          if (es[i]->getFunc(func_name).name() != e->getFunc(func_name).name()) {
            // TODO we could replace the old func_decl with the new one instead
            parts[i] = 
                funcs_equal(ctx->ctx, es[i]->getFunc(func_name), new_func_decl) &&
                parts[i];
            es[i]->getFunc(func_name) = new_func_decl;
          }
        }
      }
    }
    return ActionResult(es[0], z3::mk_or(parts));
  }
  else if (Assign* action = dynamic_cast<Assign*>(a.get())) {
    Apply* apply = dynamic_cast<Apply*>(action->left.get());
    assert(apply != NULL);

    Const* func_const = dynamic_cast<Const*>(apply->func.get());
    assert(func_const != NULL);
    func_decl orig_func = e->getFunc(func_const->name);

    z3::sort_vector domain(ctx->ctx);
    for (int i = 0; i < orig_func.arity(); i++) {
      domain.push_back(orig_func.domain(i));
    }
    func_decl new_func = ctx->ctx.function(orig_func.name(), domain, orig_func.range());

    z3::expr_vector qvars(ctx->ctx);
    z3::expr_vector all_eq_parts(ctx->ctx);
    std::unordered_map<std::string, z3::expr> vars;
    for (int i = 0; i < orig_func.arity(); i++) {
      shared_ptr<Value> arg = apply->args[i];
      if (Var* arg_var = dynamic_cast<Var*>(arg.get())) {
        expr qvar = ctx->ctx.constant(arg_var->name.c_str(), ctx->getSort(arg_var->sort));
        qvars.push_back(qvar);
        vars.insert(make_pair(arg_var->name, qvar));
      } else {
        expr qvar = ctx->ctx.constant("arg", domain[i]);
        qvars.push_back(qvar);
        all_eq_parts.push_back(qvar == e->value2expr(arg, consts));
      }
    }

    std::unordered_map<std::string, z3::func_decl> new_mapping = e->mapping;
    new_mapping.insert(make_pair(func_const->name, new_func));
    ModelEmbedding* new_e = new ModelEmbedding(ctx, new_mapping);

    return ActionResult(shared_ptr<ModelEmbedding>(new_e),
        z3::forall(qvars, new_func(qvars) == z3::ite(
          z3::mk_and(all_eq_parts),
          e->value2expr(action->right, consts, vars),
          orig_func(qvars))));
  }
  else {
    assert(false && "applyAction does not implement this unknown case");
  }
}
