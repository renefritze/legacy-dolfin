from dolfin import *

# FIXME: Plotting of vector field in Viper seems not to work

# Plot velocity
#u = Function("velocity.xml")
#plot(u)

# Plot pressure
p = Function("pressure.xml")
plot(p)
