import numpy as np
import random

class Node:
	def __init__(self, num_actions):
		self.regret_sum = np.zeros(num_actions)
		self.strategy = np.zeros(num_actions)
		self.strategy_sum = np.zeros(num_actions)
		self.num_actions = num_actions

	def compute_strategy(self):
		normalizing_sum = 0
		for a in range(self.num_actions):
			if self.regret_sum[a] > 0:
				self.strategy[a] = self.regret_sum[a]
			else:
				self.strategy[a] = 0
			normalizing_sum += self.strategy[a]

		for a in range(self.num_actions):
			if normalizing_sum > 0:
				self.strategy[a] /= normalizing_sum
			else:
				self.strategy[a] = 1.0/self.num_actions

		return self.strategy

	def get_average_strategy(self):
		avg_strategy = np.zeros(self.num_actions)
		normalizing_sum = 0
		#print('SUM: ', self.strategy_sum)
		for a in range(self.num_actions):
			normalizing_sum += self.strategy_sum[a]
		for a in range(self.num_actions):
			if normalizing_sum > 0:
				avg_strategy[a] = self.strategy_sum[a] / normalizing_sum
			else:
				avg_strategy[a] = 1.0 / self.num_actions
		
		return avg_strategy

class KuhnCFR:
	def __init__(self, iterations, decksize):
		self.nbets = 2
		self.iterations = iterations
		self.decksize = decksize
		self.cards = np.arange(decksize)
		self.bet_options = 2
		self.nodes = {}

	def cfr_iterations_external(self):
		util = np.zeros(2)
		for t in range(1, self.iterations + 1): 
			if (t%100000==0):
				print('Iteration:', t)
			for i in range(2):
				random.shuffle(self.cards)
				util[i] += self.external_cfr(self.cards[:2], [], 2, 0, i, t)
				#print(i, util[i])
				#print('Cards: ', self.cards[:2])
		print('Average game value: {}'.format(util[0]/(self.iterations)))
		for i in sorted(self.nodes):
			print(i, self.nodes[i].get_average_strategy())

	def external_cfr(self, cards, history, pot, nodes_touched, traversing_player, t):
		
		#print(cards, history, pot)
		plays = len(history)
		acting_player = plays % 2
		opponent_player = 1 - acting_player
		#print('DEPTH:', nodes_touched)
		#print('cards:', cards[acting_player])
		#print('History:', history)
		if plays >= 2: #Check payoff if history not 0, 1
			if history[-1] == 0 and history[-2] == 1: #bet fold
				if acting_player == traversing_player:
					#print("return 1")
					return 1
				else:
					#print("return -1")
					return -1
			if (history[-1] == 0 and history[-2] == 0) or (history[-1] == 1 and history[-2] == 1): #check check or bet call, go to showdown
				if acting_player == traversing_player:
					if cards[acting_player] > cards[opponent_player]:
						#print("return pot/2")
						return pot/2 #profit
					else:
						#print("return -pot/2")
						return -pot/2
				else:
					if cards[acting_player] > cards[opponent_player]:
						#print("return -pot/2 2")
						return -pot/2
					else:
						#print("return pot/2 2")
						return pot/2

		infoset = str(cards[acting_player]) + str(history)
		if infoset not in self.nodes:
			self.nodes[infoset] = Node(self.bet_options)

		nodes_touched += 1

		if acting_player == traversing_player:
			util = np.zeros(self.bet_options) #2 actions
			node_util = 0
			strategy = self.nodes[infoset].compute_strategy()
			for a in range(self.bet_options):
				#print("a "+str(a))
				next_history = history + [a]
				pot += a
				#print("nextHIstory1 "+str(next_history))
				util[a] = self.external_cfr(cards, next_history, pot, nodes_touched, traversing_player, t)
				#print("nextHIstory1 "+str(next_history)+" DONE")
				#print("strats1: ")
				#print(strategy)
				node_util += strategy[a] * util[a]

			for a in range(self.bet_options):
				regret = util[a] - node_util
				self.nodes[infoset].regret_sum[a] += regret
			#print('DEPTH', nodes_touched)
			return node_util

		else: #acting_player != traversing_player
			strategy = self.nodes[infoset].compute_strategy()
			util = 0
			if random.random() < strategy[0]:
				next_history = history + [0]
			else: 
				next_history = history + [1]
				pot += 1
			#print("nextHIstory2 "+str(next_history))
			util = self.external_cfr(cards, next_history, pot, nodes_touched, traversing_player, t)
			#print("nextHIstory2 "+str(next_history)+" DONE")
			#print("strats2: ")
			#print(strategy)
			#print('TempUtil:', util)
			
			for a in range(self.bet_options):
				self.nodes[infoset].strategy_sum[a] += strategy[a]

			#print('DEPTH', nodes_touched)
			return util

if __name__ == "__main__":
	k = KuhnCFR(10000000, 3)
	k.cfr_iterations_external()
