# jack-passthru

This is a very simple utility to create a jack client with a known name 
that can be used to play nice with some apps that work with JACK in a 
weird way (ahem, JUCE, ahem).

Anyway, the point is, some apps want to connect their ports at startup, 
even worse some apps will only create as many ports as the "device" ( 
read client) you point them at. 

Sometimes you find when you point these kinds of programs at a program 
like ardour, the port mappings get all messed up. So, the idea is to 
present a client with a given number of ports, that a given app can 
always connect its ports to, and Ardour will also remember the connections 
to this app, and everything will be happy.

## Building

You'll need (meson)[https://mesonbuild.com], and (JACK)[https://jackaudio.org/].

```
meson setup build
cd build
ninja
```

That's it.
