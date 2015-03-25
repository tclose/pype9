
/* This file was generated by NineLine version 0.1 on Tue 24 Mar 15 03:16:47PM */


#include "exceptions.h"
#include "Test.h"
#include "network.h"
#include "dict.h"
#include "integerdatum.h"
#include "doubledatum.h"
#include "dictutils.h"
#include "numerics.h"
#include "universal_data_logger_impl.h"
#include "lockptrdatum.h"

#include <limits>

using namespace nest;

/* ----------------------------------------------------------------
 * Recordables map
 * ---------------------------------------------------------------- */

nest::RecordablesMap<nineml::Test> nineml::Test::recordablesMap_;

namespace nest
{
  // Override the create() method with one call to RecordablesMap::insert_()
  // for each quantity to be recorded.
  template <>
  void RecordablesMap<nineml::Test>::create()
  {
    // use standard names whereever you can for consistency!
    insert_(names::V_m, &nineml::Test::get_V_m_);
  }
}


/* ----------------------------------------------------------------
 * Default constructors defining default parameters and state
 * ---------------------------------------------------------------- */

nineml::Test::Parameters_::Parameters_()
  : C_m    (250.0),  // pF
    I_e    (  0.0),  // nA
    tau_syn(  2.0),  // ms
    V_th   (-55.0),  // mV
    V_reset(-70.0),  // mV
    t_ref  (  2.0)   // ms
  {}

nineml::Test::State_::State_(const Parameters_& p)
  : V_m       (p.V_reset),
    dI_syn    (0.0),
    I_syn     (0.0),
    I_ext     (0.0),
    refr_count(0  )
{}

/* ----------------------------------------------------------------
 * Parameter and state extractions and manipulation functions
 * ---------------------------------------------------------------- */

void nineml::Test::Parameters_::get(DictionaryDatum &d) const
{
  (*d)[names::C_m    ] = C_m;
  (*d)[names::I_e    ] = I_e;
  (*d)[names::tau_syn] = tau_syn;
  (*d)[names::V_th   ] = V_th;
  (*d)[names::V_reset] = V_reset;
  (*d)[names::t_ref  ] = t_ref;
}

void nineml::Test::Parameters_::set(const DictionaryDatum& d)
{
  updateValue<double>(d, names::C_m, C_m);
  updateValue<double>(d, names::I_e, I_e);
  updateValue<double>(d, names::tau_syn, tau_syn);
  updateValue<double>(d, names::V_th   , V_th);
  updateValue<double>(d, names::V_reset, V_reset);
  updateValue<double>(d, names::t_ref, t_ref);

  if ( C_m <= 0 )
    throw nest::BadProperty("The membrane capacitance must be strictly positive.");

  if ( tau_syn <= 0 )
    throw nest::BadProperty("The synaptic time constant must be strictly positive.");

  if ( V_reset >= V_th )
    throw nest::BadProperty("The reset potential must be below threshold.");

  if ( t_ref < 0 )
    throw nest::BadProperty("The refractory time must be at least one simulation step.");
}

void nineml::Test::State_::get(DictionaryDatum &d) const
{
  // Only the membrane potential is shown in the status; one could show also the other
  // state variables
  (*d)[names::V_m] = V_m;
}

void nineml::Test::State_::set(const DictionaryDatum& d, const Parameters_& p)
{
  // Only the membrane potential can be set; one could also make other state variables
  // settable.
  updateValue<double>(d, names::V_m, V_m);
}

nineml::Test::Buffers_::Buffers_(Test &n)
  : logger_(n)
{}

nineml::Test::Buffers_::Buffers_(const Buffers_ &, Test &n)
  : logger_(n)
{}


/* ----------------------------------------------------------------
 * Default and copy constructor for node
 * ---------------------------------------------------------------- */

nineml::Test::Test()
  : Node(),
    P_(),
    S_(P_),
    B_(*this)
{
  recordablesMap_.create();
}

nineml::Test::Test(const Test& n)
  : Node(n),
    P_(n.P_),
    S_(n.S_),
    B_(n.B_, *this)
{}

/* ----------------------------------------------------------------
 * Node initialization functions
 * ---------------------------------------------------------------- */

void nineml::Test::init_state_(const Node& proto)
{
  const Test& pr = downcast<Test>(proto);
  S_ = pr.S_;
}

void nineml::Test::init_buffers_()
{
  B_.spikes.clear();    // includes resize
  B_.currents.clear();  // include resize
  B_.logger_.reset(); // includes resize
}

void nineml::Test::calibrate()
{
  B_.logger_.init();

  const double h  = Time::get_resolution().get_ms();
  const double eh = std::exp(-h/P_.tau_syn);
  const double tc = P_.tau_syn / P_.C_m;

  // compute matrix elements, all other elements 0
  V_.P11 = eh;
  V_.P22 = eh;
  V_.P21 = h * eh;
  V_.P30 = h / P_.C_m;
  V_.P31 = tc * ( P_.tau_syn - (h+P_.tau_syn) * eh );
  V_.P32 = tc * ( 1 - eh );
  // P33_ is 1

  // initial value ensure normalization to max amplitude 1.0
  V_.pscInitialValue = 1.0 * numerics::e / P_.tau_syn;

  // refractory time in steps
  V_.t_ref_steps = Time(Time::ms(P_.t_ref)).get_steps();
  assert(V_.t_ref_steps >= 0);  // since t_ref_ >= 0, this can only fail in error
}

/* ----------------------------------------------------------------
 * Update and spike handling functions
 * ---------------------------------------------------------------- */

void nineml::Test::update(Time const& slice_origin,
                                   const nest::long_t from_step,
                                   const nest::long_t to_step)
{
  for ( long lag = from_step ; lag < to_step ; ++lag )
  {
    // order is important in this loop, since we have to use the old values
    // (those upon entry to the loop) on right hand sides everywhere

    // update membrane potential
    if ( S_.refr_count == 0 )  // neuron absolute not refractory
      S_.V_m +=   V_.P30 * ( S_.I_ext + P_.I_e )
                  + V_.P31 * S_.dI_syn
                  + V_.P32 * S_.I_syn;
    else
     --S_.refr_count;  // count down refractory time

    // update synaptic currents
    S_.I_syn   = V_.P21 * S_.dI_syn + V_.P22 * S_.I_syn;
    S_.dI_syn *= V_.P11;

    // check for threshold crossing
    if ( S_.V_m >= P_.V_th )
    {
      // reset neuron
      S_.refr_count = V_.t_ref_steps;
      S_.V_m        = P_.V_reset;

      // send spike
      SpikeEvent se;
      network()->send(*this, se, lag);
    }

    // add synaptic input currents for this step
    S_.dI_syn += V_.pscInitialValue * B_.spikes.get_value(lag);

    // set new input current
    S_.I_ext = B_.currents.get_value(lag);

    // log membrane potential
    B_.logger_.record_data(slice_origin.get_steps()+lag);
  }
}

void nineml::Test::handle(SpikeEvent & e)
{
  assert(e.get_delay() > 0);

  B_.spikes.add_value(e.get_rel_delivery_steps(network()->get_slice_origin()),
                      e.get_weight());
}

void nineml::Test::handle(CurrentEvent& e)
{
  assert(e.get_delay() > 0);

  B_.currents.add_value(e.get_rel_delivery_steps(network()->get_slice_origin()),
		                    e.get_weight() * e.get_current());
}

// Do not move this function as inline to h-file. It depends on
// universal_data_logger_impl.h being included here.
void nineml::Test::handle(DataLoggingRequest& e)
{
  B_.logger_.handle(e);  // the logger does this for us
}
