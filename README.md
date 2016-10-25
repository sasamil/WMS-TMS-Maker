Similar to GeoWebCache, this program creates tile cache from an existing wms-source. Moreover, it creates valid OSGeo TMS and OGC WMS services upon it. After all, after executing it, there will be some HTML sample files, containing verious web-mapping JavaScript code (OpenLayers 2 and 3, ESRI JavaScript API). My intension was to illustrate how simple is to use these services.

Initially, I intended to make a raw C program, for this purpose. However, the problematics itself has drawn me towards C++, boost, templates, metaprogrammings... The version presented here is without threads and cache-update options.  I hope it will be usefull, anyway.

This code has been compiled in g++, MinGW and Microsoft's VS compilers. It has been working on both Linux and Windows. The prerequisite for building it is boost and libjpeg libraries, properly installed. This software also applies HappyHTTP library (scumways.com/happyhttp/happyhttp.html). Thanks to Ben Campbell for his code; it has been really usefull.

As I've already said, there is a multitreading version (better performancies) and a version with cache-update options (imo, cache-updating is even more frequently needed than cache-creating). More or less, this part of job is over but I am still testing these versions. 

Regarding this code or caching in general, if there are questions or (even better) suggestions - please, let me know. Don't hesitate to contact me.


