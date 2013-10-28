linear-gp
=========

What is it?
------------

It is linear genetic programming for evolving system call sequences. This GP library is coded as a part of my PhD thesis, between 2006 and 2009. Basically, this library generates an arms race between an artificial attacker (i.e. the linear GP) and an anomaly detector that builds normal behaviour models from sequences of events, in this case system calls. The genetic programming learns from the detector feedback (in the form of anomaly rates) to build 'better' attacks with lower anomaly rates.

I used the linear-gp here to perform the experiments, which are published in Applied Soft Computing in 2011:

Kayacik, H. G., Zincir-Heywood, A. N., Heywood, M. I., ["Can a Good Offense be a Good Defense? Vulnerability Testing of Anomaly Detectors Through an Artificial Arms Race"](http://web.cs.dal.ca/~kayacik/papers/ASOC10.pdf), Applied Soft Computing Journal, ISSN:1568-4946, Elsevier, 2010. 

I have recently decided to tidy up the code a bit so that I can use it to generate other types of arms race in my academic research. I am especially interested in improving detection on mobile devices through an arms race.

If you have any questions about the code, you can get in touch with me at kayacik_(at)_gmail_(dot)_com.

Running
--------
Compile all the code with the following:
	make SyscallExperiment
This should create the executable SyscallExperiment.

Platforms
---------

Linux Mandriva 2006, Mac OS 10.8 but should compile well on other platforms. 

Version History
---------------

Please note that the version numbers below are pre-git. This is just a record of changes I made to add more functions during my research.

* x.1 Implements the seeding of population with open - write - close.

* x.2 Support of multiple vulnerable applications
Implemented cut and splice XO
Fixed the Population::check_valid();
Moved the reporting code.

* x.3 Cut & Splice implements a max. individual size limit (1000).

* x.4 Instructions are selected with a probability density function.
Either PDF is derived from normal behavior or from the composition
of each individual separately.

* x.5 PDF from the composition of each individual is implemented in this version 
x.5.1 Children replace parents if they have better fitness. This version is not directly related to >x.5.

* x.6 Implements the Pareto Ranking: NSGA2's O(N^2) ranking with Kumar's PCGA
Changes in reporting, now anomaly rate without preamlbe is also reported.

* x.7 Includes the samba application.

* x.8 Supports the additional detector pH (which is under source/myPH)

* x.9 pHmr experiments were not producing correct results, that is fixed. Now pHmr has one fitness function
