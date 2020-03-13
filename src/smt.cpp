#include "smt.h"

#include <fstream> 

#include "benchmarking.h"

using namespace std;

bool enable_smt_logging = false;
extern int run_id;

namespace smt {

void log_to_stdout(long long ms, string const& log_info, string const& res) {
  #ifdef SMT_CVC4
  cout << "SMT result (cvc4) '";
  #else
  cout << "SMT result (z3) '";
  #endif
  cout << log_info << "' : " << res << " " << ms << " ms" << endl;
}

#ifdef SMT_CVC4

bool been_set = false;

string res_to_string(CVC4::Result::Sat res) {
  if (res == CVC4::Result::SAT) {
    return "sat";
  } else if (res == CVC4::Result::UNSAT) {
    return "unsat";
  } else if (res == CVC4::Result::SAT_UNKNOWN) {
    return "timeout/unknown";
  } else {
    assert(false);
  }
}

bool solver::check_sat()
{
  //cout << "check_sat" << endl;
  auto t1 = now();
  CVC4::Result::Sat res = smt.checkSat().isSat();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  assert (res == CVC4::Result::SAT || res == CVC4::Result::UNSAT);
  //cout << "res is " << (res == CVC4::Result::SAT ? "true" : "false") << endl;
  return res == CVC4::Result::SAT;
}

bool solver::is_sat_or_unknown()
{
  //cout << "is_sat_or_unknown" << endl;
  auto t1 = now();
  CVC4::Result::Sat res = smt.checkSat().isSat();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  return res == CVC4::Result::SAT || res == CVC4::Result::SAT_UNKNOWN;
}

bool solver::is_unsat_or_unknown()
{
  //cout << "is_unsat_or_unknown" << endl;
  auto t1 = now();
  CVC4::Result::Sat res = smt.checkSat().isSat();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  return res == CVC4::Result::UNSAT || res == CVC4::Result::SAT_UNKNOWN;
}

#else

string res_to_string(z3::check_result res) {
  if (res == z3::sat) {
    return "sat";
  } else if (res == z3::unsat) {
    return "unsat";
  } else if (res == z3::unknown) {
    return "timeout/unknown";
  } else {
    assert(false);
  }
}

bool solver::check_sat()
{
  auto t1 = now();
  z3::check_result res = z3_solver.check();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  assert (res == z3::sat || res == z3::unsat);
  return res == z3::sat;
}

bool solver::is_sat_or_unknown()
{
  auto t1 = now();
  z3::check_result res = z3_solver.check();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  assert (res == z3::sat || res == z3::unsat || res == z3::unknown);
  return res == z3::sat || res == z3::unknown;
}

bool solver::is_unsat_or_unknown()
{
  auto t1 = now();
  z3::check_result res = z3_solver.check();
  auto t2 = now();

  long long ms = as_ms(t2 - t1);
  log_to_stdout(ms, log_info, res_to_string(res));
  if (enable_smt_logging) {
    log_smtlib(ms, res_to_string(res));
  }

  assert (res == z3::sat || res == z3::unsat || res == z3::unknown);
  return res == z3::unsat || res == z3::unknown;
}

#endif

void solver::log_smtlib(
    long long ms,
    std::string const& res)
{
  static int log_num = 1;

  string filename = "./logs/smtlib/log." + to_string(run_id) + "." + to_string(log_num) + ".z3";
  log_num++;

  ofstream myfile;
  myfile.open(filename);
  myfile << "; time: " << ms << " ms" << endl;

  myfile << "; result: " << res;

  myfile << "; " << log_info << endl;
  myfile << endl;

  #ifdef SMT_CVC4
  smt.setInfo("smt-lib-version", 2);
  myfile << "not implemented" << endl;
  #else
  myfile << z3_solver << endl;
  #endif

  myfile.close();

  cout << "logged smtlib to " << filename << endl;
}

}
