Cabbage. A framework for developing audio instruments using Csound.

This repo contains the source code for build projects for Cabbage. Details on how to use Cabbage can be found [here](http://cabbageaudio.com).
Details on building Cabbage can be found the [Builds](https://github.com/rorywalsh/cabbage/tree/master/Builds) folder. Furter details about the builds process can be found in the continuous integration scripts for [OSX](https://github.com/rorywalsh/cabbage/blob/master/.travis.yml) and [Windows](https://github.com/rorywalsh/cabbage/blob/master/appveyor.yml). Each of these CI scripts starts with a completely new OS image and proceeds to download and install all the dependencies needed before building Cabbage and creating an installer. If you continue to have issues building please ask on the Cabbage [forum](http://forum.cabbageaudio.com). You may also file a GitHub issue, but in general, posts to the forum are seen quicker.   

Note Cabbage currently builds with JUCE 5.2.0, attempting to build with older or newer version of JUCE might lead to problems. 

<a href="https://scan.coverity.com/projects/rorywalsh-cabaiste">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/11367/badge.svg"/>
       
[![Build Status](https://travis-ci.org/rorywalsh/cabbage.svg?branch=master)](https://travis-ci.org/rorywalsh/cabbage)
