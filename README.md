## ECOZ2

Linear Predictive Coding Vector Quantization and
Hidden Markov Modeling for speech recognition.

This project is a revision of an implementation I did as part
of my BS thesis[^ecoz] on isolated word speech recognition
using LPC vector quantization[^juang82] and HMM[^rab83] [^rab89].
This implementation was directly based on the algorithms
described in the literature.

Except as indicated in a couple of places, and apart from some style
adjustments and modernization (in particular, better alignment with C99),
the implementation of the various algorithms is essentially as
originally written.
A significant improvement in this revision, however, is the use of scaling
factors[^shen] [^stamp] in HMM operations such that the system can
deal with larger models, longer observation sequences, and
larger vocabulary sizes.

### Status

The programs are already usable.
Some pending implementation aspects include some better automation
regarding model tuning, and perhaps use of a more flexible and
interoperable file format for various generated artifacts
(predictor files, quantized observation sequences, codebooks and HMM models).
Also, note that large scale model training and tuning have not been
considered at all.

*Documentation?* Sorry, There's no documentation (yet).
But you can take a look at how the programs are used in some exercises
under https://github.com/ecoz2/.

*References.*  Some key literature references are already mentioned here
and in some parts of the code, but I haven't really addressed this
aspect in any systematic way, so I'm likely missing some!
(This would be a straightforward task if I had a copy of my BS thesis
at hand, but, alas, I don't at the moment!)


### Building the programs

This is a pretty straightforward Makefile-based project with
no dependencies other than standard C libraries.
Under a Unix-like environment just type:

```
$ make
```

The programs will be placed under `_out/bin/`.

The main programs are:

```
 lpc            - Performs LPC on wav files.
 vq.learn       - Trains codebooks for vector quantization
 vq.quantize    - Generates observation sequences
 hmm.learn      - Trains HMM model
 hmm.classify   - Performs HMM based classification of observation sequences
 vq.classify    - Performs VQ based classification of predictor files
```

Other utilities:

```
 sgn.show       - Displays info about wav file
 sgn.endp       - Performs endpoint detection
 prd.show       - Displays info about generated predictor files
 vq.show        - Displays info about generated codebook
 hmm.show       - Displays info about generated HMM
 seq.show       - Displays info about observation and state sequences
```

The programs will print a basic usage message when called with no arguments.

There's no program installation per se, but you can simply include
`_out/bin` in your `$PATH`:

```
$ export PATH=`pwd`/_out/bin:$PATH
```

I have added some [plotting utilities](src/py/). 
These are python based requiring `pandas` and `matplotlib`:

```
$ pip install pandas matplotlib 
``` 

You may want to add `src/py` to your `$PATH` as well:

```
$ export PATH=`pwd`/src/py:$PATH
```

Codebook related:

```
cb.plot_evaluation.py        - Distortion, σ-ratio, inertia
cb.plot_cards_dists.py       - Cell cardinality and distortion plots
cb.plot_reflections.py       - Reflection coefficient scatter plot
                               to visualize clusters and centroids
```

----

[^ecoz]: Rueda, C.A.,
"Implementation and Experimentation with Speech Recognition
–Isolated Digits– Using LPC Vector
Quantization and Hidden Markov Models," BS. Thesis,
Systems Engineering Dept.,
Universidad Autónoma de Manizales, Colombia, 1993.

[^juang82]: Juang, B-H., Wong, D.Y., Gray, A.H.,
"Distortion Performance of Vector Quantization for LPC Voice Coding,"
IEEE Trans. ASSP, Vol. 30, No. 2, April, 1982.

[^rab83]: Rabiner, L. R., Levinson, S.D., Sondhi, M.M.,
"On the Application of Vector Quantization and Hidden Markov Models to
Speaker-Independent, Isolated Word Recognition,"
The Bell System Technical Journal, Vol. 62, No.4, April 1983.

[^rab89]: Rabiner, L. R., "A tutorial on hidden Markov models and
selected applications in speech recognition,"
Proceedings of the IEEE, Vol. 77, No. 2, 1989.

[^shen]: Shen, D., "Some Mathematics for HMM,"
http://courses.media.mit.edu/2010fall/mas622j/ProblemSets/ps4/tutorial.pdf

[^stamp]: Stamp, M., "A Revealing Introduction to Hidden Markov
Models," Computer Science Dept, San Jose State University,
https://www.cs.sjsu.edu/~stamp/RUA/HMM.pdf
