"""
  Author: Thomas G. Close (tclose@oist.jp)
  Copyright: 2012-2014 Thomas G. Close.
  License: This file is part of the "NineLine" package, which is released under
           the MIT Licence, see LICENSE for details.
"""
from __future__ import absolute_import
import pyNN.neuron.standardmodels.synapses
from pype9.common.network.synapses import StaticSynapse, ElectricalSynapse


class StaticSynapse(StaticSynapse,
                    pyNN.neuron.standardmodels.synapses.StaticSynapse):
    pass


class ElectricalSynapse(ElectricalSynapse,
                        pyNN.neuron.standardmodels.synapses.ElectricalSynapse):
    pass
