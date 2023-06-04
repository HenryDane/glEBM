# glGCM

My attempt at fitting a simple GCM into an OpenGL compute shader. 

## Processes

**Insolation process:**

$$ 
C \frac{\partial T(x, y)}{\partial t} = (1-\alpha) Q 
$$

**Albedo:**

$$ 
\alpha\left(\phi, T(x, y, z) \right) = \left\{\begin{array}{ccc}
\alpha_0 + \alpha_2 P_2(\sin\phi) & ~ & T(x, y, z) > T_f  & \text{(no ice)} \\
a_i & ~ & T(x, y, z) \le T_f & \text{(ice-covered)} \end{array} \right. 
$$

**Instant insolation:**

$$ 
Q\left(x, y, z\right) = \left\{\begin{array}{cc}
S_0 \left( \frac{\overline{d}}{d} \right)^2 \Big( \sin \phi \sin \delta + \cos\phi \cos\delta \cos h  \Big) & ~ & h > h_{0} \\
0 & ~ & h \le h_{0} \end{array} \right. 
$$

**Boltzmann radiation:**

$$ 
C \frac{\partial T(x, y)}{\partial t} = - \left( A + B~T \right) 
$$

**Meridional diffusion:**

$$ 
C \frac{\partial T(y)}{\partial t} = \frac{D}{\cos⁡\phi } \frac{\partial }{\partial \phi} \left(   \cos⁡\phi  ~ \frac{\partial T(y)}{\partial \phi} \right) 
$$

**Zonal diffusion:**

$$ 
C \frac{\partial T(x)}{\partial t} = D \frac{\partial }{\partial \lambda} \left( \frac{\partial T(x)}{\partial \lambda} \right) 
$$

**Vertical diffusion:**

$$ 
C \frac{\partial T(z)}{\partial t} = D \frac{\partial }{\partial z} \left( \frac{\partial T(z)}{\partial z} \right) 
$$

**State variables:**
 - $T(x, y, z)$ - Temperature
 - $q(x, y, z)$ - Specific humidity

**Reference data:**
 - coords(x, y, z) - Actual coordinate information (lat, lon, lev) at that spot.
 - $V(x, y, z)$ - The volume of the grid cell.

**Program struture**

`state_texture` has shape (lon,lat,lev) and contents (T, q, ?, ?) and is `RGBA32F`.

`surf_texture` has shape (lon, lat) and contents (Q, albedo, Ts, ?) and is `RGBA32F`.

Update process is:
 - update `surf_texture` into `tmp`
 - swap `tmp` --> `surf_texture`
 - (TODO) update `state_texture` somehow
 - render surf temp
 
 




