package tictactoe

import "errors"

type Player int

const (
	Empty Player = iota
	X
	O
)

func (p Player) Opponent() Player {
	switch p {
	case X:
		return O
	case O:
		return X
	default:
		return Empty
	}
}

type Board [9]Player

func NewBoard() Board {
	return Board{}
}

func (b Board) IsFull() bool {
	for _, cell := range b {
		if cell == Empty {
			return false
		}
	}
	return true
}

func (b Board) Winner() Player {
	lines := [8][3]int{
		{0, 1, 2},
		{3, 4, 5},
		{6, 7, 8},
		{0, 3, 6},
		{1, 4, 7},
		{2, 5, 8},
		{0, 4, 8},
		{2, 4, 6},
	}

	for _, line := range lines {
		if b[line[0]] != Empty &&
			b[line[0]] == b[line[1]] &&
			b[line[1]] == b[line[2]] {
			return b[line[0]]
		}
	}

	return Empty
}

func (b Board) IsTerminal() bool {
	return b.Winner() != Empty || b.IsFull()
}

type Game struct {
	Board  Board
	Turn   Player
	Winner Player
}

func NewGame(first Player) *Game {
	if first != X && first != O {
		first = X
	}

	return &Game{
		Turn: first,
	}
}

func (g *Game) Move(position int) error {
	if position < 0 || position >= len(g.Board) {
		return errors.New("position must be between 0 and 8")
	}
	if g.Winner != Empty || g.Board.IsFull() {
		return errors.New("game is over")
	}
	if g.Board[position] != Empty {
		return errors.New("position is occupied")
	}

	g.Board[position] = g.Turn
	g.Winner = g.Board.Winner()

	if g.Winner == Empty && !g.Board.IsFull() {
		g.Turn = g.Turn.Opponent()
	}

	return nil
}

func BestMove(board Board, player Player) int {
	if player != X && player != O {
		return -1
	}
	if board.IsTerminal() {
		return -1
	}

	bestPosition := -1
	bestScore := -2

	for position, cell := range board {
		if cell != Empty {
			continue
		}

		next := board
		next[position] = player
		score := minimax(next, player.Opponent(), player, 0)

		if score > bestScore {
			bestScore = score
			bestPosition = position
		}
	}

	return bestPosition
}

func minimax(board Board, turn, maximizingPlayer Player, depth int) int {
	winner := board.Winner()
	if winner == maximizingPlayer {
		return 10 - depth
	}
	if winner == maximizingPlayer.Opponent() {
		return depth - 10
	}
	if board.IsFull() {
		return 0
	}

	if turn == maximizingPlayer {
		best := -100
		for position, cell := range board {
			if cell != Empty {
				continue
			}

			next := board
			next[position] = turn
			score := minimax(next, turn.Opponent(), maximizingPlayer, depth+1)
			if score > best {
				best = score
			}
		}
		return best
	}

	best := 100
	for position, cell := range board {
		if cell != Empty {
			continue
		}

		next := board
		next[position] = turn
		score := minimax(next, turn.Opponent(), maximizingPlayer, depth+1)
		if score < best {
			best = score
		}
	}
	return best
}
