Similar to GeoWebCache, this program creates tile cache from an existing wms-source. Moreover, it generates valid OSGeo TMS and OGC WMS services upon it. After all, after executing it, there will be some HTML sample files, containing corresponding web-maps and verious JavaScript code (OpenLayers 2 and 3, ESRI JavaScript API). My intension was to illustrate, by samples, the simplicity of using/applying the resulting services.

Initially, I had wished to develop a C-style program, for this purpose. And - I started that way, frankly. However, the problematics itself was constantly drawing me towards C++, boost, templates, metaprogrammings... So, I've finally come to the version presented here.  It's not final; the project is ongoing and there are other versions I'm working on. This one is without threads and cache-update options.  I hope it will be of use, anyway.

This code has been compiled in g++, MinGW and Microsoft's VS compilers. It has been working on both Linux and Windows. The prerequisite for building it is that boost and libjpeg libraries are properly installed. This software also applies HappyHTTP library (scumways.com/happyhttp/happyhttp.html). Thanks to Ben Campbell for his code; it is really neat and usefull.

As I've already said, the multitreading version exists (much better performancies) and there is a version with cache-update options (imo, cache-updating is even more frequently needed than cache-creating). More or less, these versions are over but they are still at testing phase. 

If there are questions in regards to this code/project, or caching in general - please, let me know. Suggestions will be welcomed. Don't hesitate to contact me.


