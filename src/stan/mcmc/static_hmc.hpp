#ifndef __STAN__MCMC__STATIC__HMC__BETA__
#define __STAN__MCMC__STATIC__HMC__BETA__

#include <ctime>

#include <boost/random/mersenne_twister.hpp>

#include <stan/model/prob_grad_ad.hpp>

#include <stan/mcmc/adapter.hpp>
#include <stan/mcmc/hmc_base.hpp>
#include <stan/mcmc/hamiltonian.hpp>
#include <stan/mcmc/integrator.hpp>
#include <stan/mcmc/util.hpp>

namespace stan {

  namespace mcmc {

    //////////////////////////////////////////////////
    //  HMC samplers with a static integration time //
    //////////////////////////////////////////////////
    
    // Unit metric
    
    template <typename M, class BaseRNG = boost::mt19937>
    class unit_metric_hmc: public hmc_base<M, 
                                           unit_metric, 
                                           expl_leapfrog, 
                                           BaseRNG> {
      
    public:
      
      unit_metric_hmc(M &m);
      ~unit_metric_hmc() {};
      
      virtual double sample(std::vector<double>& q, std::vector<int>& r);
      
      void set_stepsize_and_T(const double e, const double t) {
        _epsilon = e; _T = t; _update_L();
      }
      
      void set_stepsize_and_L(const double e, const double l) {
        _epsilon = e; _L = l; _T = _epsilon * _L;
      }
      
      void set_T(const double t) { _T = t; _update_L(); }
      void set_stepsize(const double e) { _epsilon = e; _update_L(); }
      
      void get_stepsize() { return _epsilon; }
      void get_L() { return _L; }
      void get_T() { return _T; }
      
    protected:
      
      double _epsilon;
      double _L;
      double _T;
      
      void _update_L() { _L = static_cast<int>(_T / _epsilon); }
                        
    };
    
    template <typename M, class BaseRNG>
    unit_metric_hmc<M, BaseRNG>::unit_metric_hmc(M& m):
    hmc_base<M, unit_metric, expl_leapfrog, BaseRNG>(m),
    _epsilon(0.1),
    _T(1)
    { _update_L(); }
    
    template <typename M, class BaseRNG>
    double unit_metric_hmc<M, BaseRNG>::sample(std::vector<double>& q, std::vector<int>& r) {
      
      ps_point z(q.size());
      z.q = q;
      z.r = r;
      
      Eigen::VectorXd u(q.size());
      for (int i = 0; i < u.size(); ++i) u(i) = this->_rand_unit_gaus();
      
      this->_hamiltonian.sampleP(z, u);
      this->_hamiltonian.init(z);
      
      double H0 = this->_hamiltonian.H(z);
      
      for(int i = 0; i < _L; ++i)
      {
        this->_integrator.evolve(z, this->_hamiltonian, _epsilon);
      }
      
      double acceptProb = exp(H0 - this->_hamiltonian.H(z));
      
      double accept = false;
      if(acceptProb > 1 || this->_rand_uniform() < acceptProb) 
      {
        q = z.q;
        accept = true;
      }
      
      return accept;
      
    }
    
    template <typename M, class BaseRNG = boost::mt19937>
    class adapt_unit_metric_hmc: public unit_metric_hmc<M, BaseRNG>, adapter {
      
    public:
      
      adapt_unit_metric_hmc(M &m);
      ~adapt_unit_metric_hmc() {};
      
      double sample(std::vector<double>& q, std::vector<int>& r);
      
    };

    template <typename M, class BaseRNG>
    adapt_unit_metric_hmc<M, BaseRNG>::adapt_unit_metric_hmc(M& m):
    unit_metric_hmc<M, BaseRNG>(m),
    adapter()
    {};
    
    template <typename M, class BaseRNG>
    double adapt_unit_metric_hmc<M, BaseRNG>::sample(std::vector<double>& q, std::vector<int>& r) {
      
      double accept_stat = unit_metric_hmc<M, BaseRNG>::sample(q, r);
      
      this->_learn_stepsize(this->_epsilon, accept_stat);
      this->_update_L();
      
      return accept_stat;
      
    }
    
    /*
    //////////////////////////////////////////////////
    //     Basic Leapfrog with Custom Mass Matrix   //
    //////////////////////////////////////////////////
    
    class EMHMC: public hmcSampler<denseConstMetric>
    {
      
    public:
      
      EMHMC(double epsilon, double L, model &m);
      
      void sample(VectorXd& q);
      
    private:
      
      double _epsilon;
      double _L;
      
    }
    
    EMHMC::EMHMC(double epsilon, double L, model& m):
    hmcSampler(m),
    _epsilon(epsilon),
    _L(L)
    {}
    
    void EMHMC::sample(VectorXd& q)
    {
      
      _hamiltonian.Minv = MatrixXd::Ones(model.dim(), model.dim());
      
      ps_point z(model.dim());
      z.q = q;
      _hamiltonian.sampleP(z.p, RNG);
      
      double H0 = _hamiltonian.H(z);
      
      for(int i = 0; i < _L; ++i)
      {
        _evolve.evolve(z, _hamiltonian, _epsilon);
      }
      
      double acceptProb = exp(H0 - _hamiltonian.H(z));
      
      if(RNG.U() < acceptProb) q = z.q();
      
      return;
      
    }
     */

    
  } // mcmc

} // stan
          

#endif
