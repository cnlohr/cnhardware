# General notes


Current Approach: Expected 3.1 to 3.9x bitrate.  I.e. Target 1MSPS sampling.

## Process

1. Get sample rate right with ADC
2. Export and graph raw ADC values
3. Come up with a bit-detection scheme for the BMC coding
(we are here)
4. ???
5. Turn decoding into a table (x16, where output is in MSByte, feedback is in LSB)


## Resources

https://eeucalyptus.net/2023-12-06-usb-pd-1.html


## Decoding Regimes

### 2.5x

2.5x ( 750k - 900k ) (not inclusive)

Will yield (@820kSPS)

```
runs, bit, bit size
1 1 2
3 0 3
1
1 1 2
3 0 3
1
2 1 3
2 0 2
1
2 1 3
2 0 2
2 \
1 / Must be paired, because you cannot have two 2 runs in a row.
```

If you exceed 750k, you can guarantee that you will never have two bits of length 2 in a row.  This is too shady for me.


### 3x

3x (920k - 1180k)

Much less ambiguous!  Always can have 1/2 = half of a start bit.

Will yield (@1.1MSPS)

```
runs, bit, bit size
3
2
1
4 0 4
1
2 1 3
4 0 4
1
2 1 3
4 0 4
1
2 1 3
3 0 3
2
2 1 4
3 0 3
2
2 1 4
3 0 3
2
1 1 3
```

### 4+x

@ 1300k

```
runs, bit, bit size
4 0 4
2
2 1 4
4 0 4
2
3 1 5
4 0 4
2
2 1 4
5 0 5
2
2 1 4
5 0 5
2
2 1 4
4 0 4
3
2 1 5
4 0 4
```

### 3-4.5x?

910k to 1350k

At bit boundary:

 * If number is 1 or 2, it must be a 1 (Expect Short)
 * If number is 3, TRICKY TRICKY
 * If number is 4+, it must be a 0 (Long bit)

This is very hard.

