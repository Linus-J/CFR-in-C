## Counterfactual Regret Minimization lightweight C implementation

### Description:
Repository for CFR algorithms written in C to optimise execution speeds for each implementation. So far MCCFR (Monte Carlo Counterfactual Regret Minimization), Vanilla CFR, CFR+ and DCFR (Discounted Counterfactual Regret Minimization) have been implemented in this repository to solve [Kuhn Poker](https://en.wikipedia.org/wiki/Kuhn_poker "Kuhn Poker"). Additionally, the DCFR algorithm has been implemented to solve a variant of Leduc Poker, which is a larger game than Kuhn poker. Functions to calculate exploitability in both settings are also included. A perfect mapping function is implemented for each CFR setting that allocates each information set to a value in a 2D array that is used to efficiently access the strategies for both players in Kuhn and Leduc Poker. The future aim is to expand the implementation of DCFR to Flop Hold'em, Limit Texas Hold'em and finally No-Limit Texas Hold'em.

<img src="https://github.com/Linus-J/repo-images/blob/main/cfr-in-c/pokerbot.png" alt="Pokerbot" width="300"/>

### Installation instructions:
#### Step 1: [Clone the repo]

`git clone https://github.com/Linus-J/CFR-in-C.git`

#### Step 2: [Compile your desired CFR implementation(s)]

`make` to compile all variants, or:

`make cfr` / `make cfrplus` / `make mccfr` / `make dcfr` / `make dcfrLeduc` 

#### Step 3: [Execute]

`./cfr` / `./cfrplus` / `./mccfr` / `./dcfr` / `./dcfrLeduc` 

#### Note:

To change the number of iterations or game type, edit the source code and remake the amended file.

### Why C?:
CFR and other algorithms written for imperfect information games are commonly written in Python. This is great for initially writing up the algorithms in a practical way, however, does have some considerable drawbacks. Often the straightforward implementation involves using abstract data/class structures that result in slow execution times. Python makes it easy to do this and lets us use dynamic variable typing so we don't have to worry about type declaration as the program writer. Python uses PyObjects to allow for dynamic variable typing and is also an interpreted language. These among other things were those qualities that helped us efficiently write out our algorithm in Python, but are now the reasons for slower execution times. 

Algorithms used in AI often have to parse a very large amount of data so over many iterations over the data, execution time and resource management issues become glaring. Instead of relentlessly using higher-performing hardware to run suboptimal software, it is first necessary to trim the necessary fat off the software itself.

C is a procedural programming language that allows you to have a lot more control over the exact data types of variables you wish to use as well as memory management. After it is written, C source code is compiled allowing the resulting executable file to have low-level capabilities that allow for efficient access to machine instructions. For this reason, C is commonly used for many cases that desire its lower-level capabilities such as in operating systems, drivers, microchips and supercomputers.

Python has libraries such as NumPy and PyTorch that contain a lot of C under the hood but the interpretation of the Python still causes a loss in speed.

Projects like Cython exist that convert Python programs into a C program that can be compiled but often the C source code is extremely verbose and not streamlined like it would be if you were to carefully write the program in C from scratch yourself. Therefore, algorithms implemented in C will often result in much superior execution times if done well and this time save will scale when working with larger amounts of data, thus saving considerable time and computational resources.

### Checklist:
- External sampling CFR ([MCCFR](https://proceedings.neurips.cc/paper/2009/file/00411460f7c92d2124a67ea0f4cb5f85-Paper.pdf "MCCFR") variant): ✔️
- [Vanilla CFR](https://proceedings.neurips.cc/paper/2007/file/08d98638c6fcd194a4b1e6992063e944-Paper.pdf "Vanilla CFR"): ✔️
- [CFR+](https://arxiv.org/abs/1407.5042 "CFR+"): ✔️
- [Discounted CFR](https://ojs.aaai.org/index.php/AAAI/article/download/4007/3885 "Discounted CFR") (Discounted CFR with specific params): ✔️
- Implement Leduc poker with DCFR: 🐛
- Implement functions to compute exploitability for Kuhn poker: ✔️
- Implement functions to compute best-response for Leduc poker: 🐛
- Implement Flop Hold'em poker with DCFR: ➖
