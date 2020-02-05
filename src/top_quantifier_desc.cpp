#include "top_quantifier_desc.h"

#include <set>
#include <cassert>

using namespace std;

TopQuantifierDesc::TopQuantifierDesc(value v) {
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      for (VarDecl decl : f->decls) {
        d.push_back(make_pair(QType::Forall, vector<VarDecl>{decl}));
      }
      v = f->body;
    } else if (NearlyForall* f = dynamic_cast<NearlyForall*>(v.get())) {
      d.push_back(make_pair(QType::NearlyForall, f->decls));
      v = f->body;
    } else {
      break;
    }
  }
}

pair<TopQuantifierDesc, value> get_tqd_and_body(value v)
{
  TopQuantifierDesc tqd(v);

  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      v = f->body;
    } else if (NearlyForall* f = dynamic_cast<NearlyForall*>(v.get())) {
      v = f->body;
    } else {
      break;
    }
  }

  return make_pair(tqd, v);
}

vector<VarDecl> TopQuantifierDesc::decls() const {
  vector<VarDecl> res;
  for (auto p : d) {
    for (auto decl : p.second) {
      res.push_back(decl);
    }
  }
  return res;
}

vector<QRange> TopQuantifierDesc::with_foralls_grouped() const {
  vector<QRange> res;
  QRange cur;
  cur.start = 0;
  cur.end = 0;

  for (auto p : d) {
    QType ty = p.first;
    if (ty == QType::Forall) {
      cur.end++;
      cur.decls.push_back(p.second[0]);
      cur.qtype = ty;
    } else if (ty == QType::NearlyForall) {
      if (cur.end > cur.start) {
        res.push_back(cur);
      }

      cur.start = cur.end;
      cur.decls = p.second;
      cur.end = cur.start + cur.decls.size();
      cur.qtype = ty;
      res.push_back(cur);

      cur.start = cur.end;
      cur.decls.clear();
    } else {
      assert(false);
    }
  }

  if (cur.end > cur.start) {
    res.push_back(cur);
  }

  return res;
}

vector<QSRange> TopQuantifierDesc::grouped_by_sort() const {
  vector<QSRange> res;
  QSRange cur;
  cur.start = 0;
  cur.end = 0;

  for (auto p : d) {
    QType ty = p.first;
    for (auto decl : p.second) {
      if (cur.end > cur.start && !(cur.qtype == ty && sorts_eq(cur.decls[0].sort, decl.sort))) {
        res.push_back(cur);
        cur.start = cur.end;
        cur.decls.clear();
      }

      cur.decls.push_back(decl);
      cur.qtype = ty;
      cur.sort = decl.sort;
      cur.end++;
    }

    if (ty == QType::NearlyForall && cur.end > cur.start) {
      res.push_back(cur);
      cur.start = cur.end;
      cur.decls.clear();
    }
  }

  if (cur.end > cur.start) {
    res.push_back(cur);
  }

  set<string> names;
  for (int i = 0; i < (int)res.size(); i++) {
    if (i > 0 && res[i].qtype != res[i-1].qtype) {
      names.clear();
    }
    UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(res[i].decls[0].sort.get());
    assert(usort != NULL);
    if (res[i].qtype == QType::Forall) {
      assert (names.count(usort->name) == 0);
    }
    names.insert(usort->name);
  }

  return res;
}

value TopQuantifierDesc::with_body(value body) const {
  vector<QRange> qrs = with_foralls_grouped();
  for (int i = qrs.size() - 1; i >= 0; i--) {
    QRange& qr = qrs[i];
    if (qr.qtype == QType::Forall) {
      body = v_forall(qr.decls, body);
    }
    else if (qr.qtype == QType::NearlyForall) {
      body = v_nearlyforall(qr.decls, body);
    }
    else {
      assert(false);
    }
  }
  return body;
}

int TopQuantifierDesc::weighted_sort_count(std::string sort) const {
  lsort so = s_uninterp(sort);
  int count = 0;
  for (auto p : d) {
    QType ty = p.first;
    for (VarDecl const& decl : p.second) {
      if (sorts_eq(so, decl.sort)) {
        count += (ty == QType::NearlyForall ? 2 : 1);
      }
    }
  }
  return count;
}

TopAlternatingQuantifierDesc::TopAlternatingQuantifierDesc(value v)
{
  Alternation alt;
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      if (alt.decls.size() > 0 && alt.altType != AltType::Forall) {
        alts.push_back(alt);
        alt.decls = {};
      }

      alt.altType = AltType::Forall;

      for (VarDecl decl : f->decls) {
        alt.decls.push_back(decl);
      }

      v = f->body;
    } else if (NearlyForall* f = dynamic_cast<NearlyForall*>(v.get())) {
      assert(false);
    } else if (Exists* f = dynamic_cast<Exists*>(v.get())) {
      if (alt.decls.size() > 0 && alt.altType != AltType::Exists) {
        alts.push_back(alt);
        alt.decls = {};
      }

      alt.altType = AltType::Exists;

      for (VarDecl decl : f->decls) {
        alt.decls.push_back(decl);
      }

      v = f->body;
    } else {
      break;
    }
  }

  if (alt.decls.size() > 0) {
    alts.push_back(alt);
  }
}

value TopAlternatingQuantifierDesc::get_body(value v)
{
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      v = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(v.get())) {
      v = f->body;
    }
    else {
      break;
    }
  }
  return v;
}

value TopAlternatingQuantifierDesc::with_body(value v)
{
  for (int i = alts.size() - 1; i >= 0; i--) {
    Alternation& alt = alts[i];
    if (alt.altType == AltType::Forall) {
      v = v_forall(alt.decls, v);
    } else {
      v = v_exists(alt.decls, v);
    }
  }
  return v;
}

vector<QSRange> TopAlternatingQuantifierDesc::grouped_by_sort() const {
  vector<QSRange> res;
  QSRange cur;
  cur.start = 0;
  cur.end = 0;

  for (auto alt : alts) {
    assert(alt.altType == AltType::Forall || alt.altType == AltType::Exists);
    QType ty = alt.altType == AltType::Forall ? QType::Forall : QType::Exists;
    for (auto decl : alt.decls) {
      if (cur.end > cur.start && !(cur.qtype == ty && sorts_eq(cur.decls[0].sort, decl.sort))) {
        res.push_back(cur);
        cur.start = cur.end;
        cur.decls.clear();
      }

      cur.decls.push_back(decl);
      cur.qtype = ty;
      cur.sort = decl.sort;
      cur.end++;
    }

    /*if (ty == QType::NearlyForall && cur.end > cur.start) {
      res.push_back(cur);
      cur.start = cur.end;
      cur.decls.clear();
    }*/
  }

  if (cur.end > cur.start) {
    res.push_back(cur);
  }

  set<string> names;
  for (int i = 0; i < (int)res.size(); i++) {
    if (i > 0 && res[i].qtype != res[i-1].qtype) {
      names.clear();
    }
    UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(res[i].decls[0].sort.get());
    assert(usort != NULL);
    if (res[i].qtype == QType::Forall) {
      assert (names.count(usort->name) == 0);
    }
    names.insert(usort->name);
  }

  return res;
}
