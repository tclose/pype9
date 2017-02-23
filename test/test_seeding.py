import ninemlcatalog
from nineml import units as un
from pype9.neuron import CellMetaClass, simulation_controller
import matplotlib.pyplot as plt
from itertools import repeat
import numpy

nineml_model = ninemlcatalog.load('//input/Poisson#Poisson')
Cell = CellMetaClass(nineml_model, name='PoissonTest',
                     build_mode='force')
cell = Cell(rate=300 / un.s)
cell.set_state({'t_next': 5 * un.ms})
cell.record('spike_output')
Cell2 = CellMetaClass(nineml_model, name='PoissonTest2',
                      build_mode='force')
cell2 = Cell(rate=150 / un.s)
cell2.set_state({'t_next': 1 * un.ms})
cell2.record('spike_output')

simulation_controller.run(100 * un.ms)
cell1_rec = cell.recording('spike_output')
cell2_rec = cell2.recording('spike_output')
y, x = numpy.array(zip(repeat(1), cell1_rec) + zip(repeat(2), cell2_rec)).T
print "cell1: {}".format(len(cell1_rec))
print "cell2: {}".format(len(cell2_rec))
plt.scatter(x, y)
plt.show()
print "done"
