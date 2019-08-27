2019-09

- vq.learn: report codeword and minimum distortion for each training vector against
  each generated codebook
- `cb.plot_cell_shapes.py` utility to display the above info
  
- additions/adjustments for more organized codebook evaluation report.
  
    Let's do a complete exercise starting from a single sound file.
     
    Setup: 
    
        $ make
        $ export PATH=`pwd`/_out/bin:$PATH
        $ export PATH=`pwd`/src/py:$PATH
        $ pip install pandas matplotlib  # optional: for plotting
        
    Assuming `HBSe_20161221T010133.wav` in some directory:
    
        $ cd <some-directory>
        $ lpc -P 36 -W 45 -O 15 HBSe_20161221T010133.wav
        Number of classes: 1
        class '': 1
          HBSe_20161221T010133.wav
        lpaOnSignal: P=36 numSamples=18368474 sampleRate=32000 winSize=1440 offset=480 T=38265
        data/predictors/_/HBSe_20161221T010133.prd: '': predictor saved
        
    This generates a single predictor file. Usually a predictor file is associated
    with a single vocalization (a human spoken word or a whale song unit), but in
    this case we are just interested in aggregating a set of LPC vectors from a large
    sound file for purposes of the clustering exercise.
    
    Let's generate the codebooks:
    
        $ vq.learn -P 36 -e 0.0005 data/predictors/_/HBSe_20161221T010133.prd
    
    `vq.learn` now also generates various `.csv` files, one for the overall evaluation
    `eps_0.0005.rpt.csv` and one per codebook size with cardinalities and distortions:
    
        $ ls data/codebooks/_
        eps_0.0005.rpt
        eps_0.0005.rpt.csv
        eps_0.0005_M_0002.cbook
        eps_0.0005_M_0002.cbook.cards_dists.csv
        eps_0.0005_M_0004.cbook
        eps_0.0005_M_0004.cbook.cards_dists.csv
        eps_0.0005_M_0008.cbook
        eps_0.0005_M_0008.cbook.cards_dists.csv
        eps_0.0005_M_0016.cbook
        eps_0.0005_M_0016.cbook.cards_dists.csv
        eps_0.0005_M_0032.cbook
        eps_0.0005_M_0032.cbook.cards_dists.csv
        eps_0.0005_M_0064.cbook
        eps_0.0005_M_0064.cbook.cards_dists.csv
        eps_0.0005_M_0128.cbook
        eps_0.0005_M_0128.cbook.cards_dists.csv
        eps_0.0005_M_0256.cbook
        eps_0.0005_M_0256.cbook.cards_dists.csv
        eps_0.0005_M_0512.cbook
        eps_0.0005_M_0512.cbook.cards_dists.csv
        eps_0.0005_M_1024.cbook
        eps_0.0005_M_1024.cbook.cards_dists.csv
        eps_0.0005_M_2048.cbook
        eps_0.0005_M_2048.cbook.cards_dists.csv
    
    Let's plot the general evaluation:
    
        $ data/codebooks/_/eps_0.0005.rpt.csv
               M  passes     DDprm          σ       inertia
        0      2       7  0.329847   1.669760  85563.055636
        1      4      19  0.258477   3.662945  67812.819378
        2      8      26  0.198647   7.851806  58449.267547
        3     16      32  0.157429  11.032102  52835.579481
        4     32      16  0.130626  15.396460  49804.951420
        5     64      17  0.110648  20.375099  47672.295169
        6    128      17  0.096532  24.747489  46300.641095
        7    256      16  0.084483  30.756691  45173.663627
        8    512      15  0.074696  36.942795  44299.052234
        9   1024      14  0.065547  43.420272  43505.964302
        10  2048      12  0.056605  51.227322  42754.019571
        
    This generates `cb_evaluation.png`.
    
    Let's plot the cardinalities and distortions for M=1024:
    
        $ cb.plot_cards_dists.py data/codebooks/_/eps_0.0005_M_1024.cbook.cards_dists.csv
        
    This generates `cb_cards_dists.png`.
    
    Now, let's extract the k_1, k_2, and k_3 reflection coefficients from the
    training vectors:
    
        $ prd.show -k -r 1-3 data/predictors/_/HBSe_20161221T010133.prd > data/predictors/_/HBSe_20161221T010133.prd.kkk.csv
        $ head data/predictors/_/HBSe_20161221T010133.prd.kkk.csv
        # data/predictors/_/HBSe_20161221T010133.prd:
        # className='', T=38265, P=36
        k1,k2,k3
        -0.13743,0.321453,-0.112715
        -0.103927,0.320109,-0.137698
        -0.136486,0.322977,-0.0785831
        -0.160965,0.327485,-0.147224
        -0.127233,0.264571,-0.181007
        -0.144313,0.370183,-0.135964
        -0.0980965,0.411245,-0.101035 
    
    and also from the codebooks, say from the M=1024 codebook:

        $ cb.show -r 1-3 data/codebooks/_/eps_0.0005_M_1024.cbook > data/codebooks/_/eps_0.0005_M_1024.cbook.kkk.csv
        $ head data/codebooks/_/eps_0.0005_M_1024.cbook.kkk.csv
        # data/codebooks/_/eps_0.0005_M_1024.cbook:
        # P=36  size=1024  className='_'
        k1,k2,k3
        -0.76091,-0.0986296,-0.407168
        -0.719444,-0.0847496,-0.43053
        -0.70288,-0.0588205,-0.38793
        -0.706682,-0.00428451,-0.376763
        -0.537687,0.0408431,-0.333315
        -0.542176,0.0610915,-0.350283
        -0.609534,-8.40823e-05,-0.372286
        
    With the above set of reflection coefficients, let's do a "k_1 vs. k_2 vs. and k_3"
    scatter plot to visualize the clusters:
    
        $ cb.plot_reflections.py data/predictors/_/HBSe_20161221T010133.prd.kkk.csv data/codebooks/_/eps_0.0005_M_1024.cbook.kkk.csv
        df_training points = 38265
        df_training plotted points = 8000
        df_codebook points = 1024
 
    This generates `cb_kkk_training_8000_codebook_1024.png`. 
    
- add inertia calculation
- prd.show/cb.show: allow for coefficient range selection and generate csv

2018-09-27

- complete first general revision over selected parts of
  my original "ECOZ" implementation of 1993.
