# glGCM

My attempt at fitting a simple energy balance model into an OpenGL compute shader. 

Requires NetCDF, GLFW, and OpenGL. It may be necessary to fiddle with `LFLAGS` and `CFLAGS` inside the Makefile, depending on your system configuration.

## Processes

TODO: Add and document process switches.

### A+BT OLR

The outgoing long-wave radiation $\Phi_{OLR}$ is computed by:

```math
\Phi_{OLR} = A + B * T
```

Where $A$ and $B$ are configurable parameters.

### Insolation

TODO.

### Albedo

The albedo $\alpha$ is computed by:

```math
\alpha(T, ...) = 
\left\{\begin{matrix}
\alpha_{0} + \alpha_{2}P_{2}(\sin(\phi)) & T > T_{f}\\ 
\alpha_{i} & T \le T_{f} 
\end{matrix}\right.
```

Where $\alpha_{0}$, $\alpha_{2}$, and $\alpha_{i}$ are configurable parameters.

### Surface Temperature

The surface temperature $T$ is computed from the energy balance of OLR and ASR. The ASR is computed by:

```math
\Phi_{ASR} = (1 - \alpha) Q
```

And is then used to compute $T$ like so:

```math
C \frac{\partial T}{\partial t} = \Phi_{ASR} - \Phi_{OLR}
```

Where $C$ is the specific heat, calculated from the depth $h$ like so:

```math
C = 4184.3 \frac{\textup{J}}{\textup{m}^{2}\textup{K}} \times 1000 \frac{\textup{kg}}{\textup{m}^{3}} \times h
```

Where the depth is a configurable parameter.

### Diffusion

TODO.

### Moist Amplification Factor

The moist amplification factor $f$ is computed (and used in the advection-diffusion schema) thusly:

TODO.


## Compilation

After you have installed the necessary dependencies, simply run:

```
make
```

to compile the executable, which will be named `glEBM` by default.

## Useage

The model expects to be fed a NetCDF file with two dimenstions: `lat`, and `lon`. It uses these to pick grid cell centers and to identify the dimensions of the model. In order for the model to function, the model dimensions must both be multiples of 32. 

The following optional parameters (along with their default values) are listed below:

 - Initial surface temperature (`T`, `Ts`, `Temperature`, `temperature`). Defaults to 273.15K.
 - Parameter A for A+BT (`A`, `As`). Defaults to 210.0 W/m2.
 - Parameter B for A+BT (`B`, `Bs`). Defaults to 2.0 W/m/K.
 - Water depth (`depth`, `depths`). Defaults to 30.0 m.
 - Albedo parameter $a_{0}$ (`a0`, `a0s`). Defaults to 0.3.
 - Albedo parameter $a_{2}$ (`a2`, `a2s`). Defaults to 0.078.
 - Albedo parameter $a_{i}$ (`ai`, `ais`). Defaults to 0.62.

The model may be executed by running:

```
glEBM <path_to_input_file.nc> <path_to_output_file.nc>
```

The results will be placed into a single NetCDF file.

