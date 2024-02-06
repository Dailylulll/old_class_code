import time


class Board_Spot:
    row: int
    col: int
    move: str
    valid_moves: list[str] = [" ", "x", 'o']
    links: dict

    def __str__(self) -> str:
        return f"(r:{self.row}, c:{self.col})"

    def __repr__(self) -> str:
        return f"(r:{self.row}, c:{self.col})"

    def __init__(self, row: int, col: int):
        self.row = row
        self.col = col
        self.move = " "
        self.links = dict[str, Board_Spot]()  # u ur r dr d dl l ul

    def __eq__(self, other):
        return self.__str__() == other.__str__()

    def set_move(self, move) -> bool:
        if move not in self.valid_moves or self.move != " ":
            return False
        else:
            self.move = move
            return True

    def get_move(self) -> str:
        return self.move

    def set_link(self, direction: str, b_spot):
        if direction not in self.links:
            self.links[direction] = b_spot
        else:
            return

    def get_link(self, direction: str):
        if direction in self.links:
            return self.links[direction]
        else:
            return None

    def get_streak(self, direct: str):
        spot = self
        move = self.move
        streak: int = 0
        while True:
            spot = spot.get_link(direct)
            if spot is not None and spot.move == move:
                streak += 1
            else:
                break
        return streak

    def get_empty_adjacent(self):
        adj_list = []
        for adj_spot in self.links.values():
            if adj_spot.move == " ":
                adj_list.append(adj_spot)
        return adj_list


class Board:
    row: int
    col: int
    streak: int
    grid: list[list[Board_Spot]]
    spot_dict: dict[str, Board_Spot]
    direction_pairs = [["u", "d"], ["l", "r"], ["dr", "ul"], ["dl", "ur"]]
    vec_lists: list[list[Board_Spot]]

    def __init__(self, row: int, col: int, streak: int):
        self.row = row
        self.col = col
        self.streak = streak
        self.spot_dict = dict[str, Board_Spot]()
        self.grid = self.build_grid(row, col)
        self.init_links()
        self.vec_lists = self.build_vector_lists()
        self.h_dict = self.map_heuristic_scores()

    def __str__(self) -> str:
        string: str = ""
        for col in self.grid:
            for spot in col:
                string += spot.__str__() + " "
            string += "\n"
        return string

    def copy_board(self):
        new_board: Board = Board(self.row, self.col, self.streak)
        for r in range(self.row):
            for c in range(self.col):
                new_board.grid[r][c].move = self.grid[r][c].move
        return new_board

    def build_grid(self, row: int, col: int) -> list[list[Board_Spot]]:
        row_list: list[list[Board_Spot]] = []
        for r in range(row):
            col_list: list[Board_Spot] = []
            row_list.append(col_list)
            for c in range(col):
                b_spot: Board_Spot = Board_Spot(r, c)
                col_list.append(b_spot)
                self.spot_dict[f"r:{r}c:{c}"] = b_spot
        return row_list

    def init_links(self):
        r_domain = range(self.row)
        c_domain = range(self.col)
        c_list = [0, 1, 1, 1, 0, -1, -1, -1]
        r_list = [-1, -1, 0, 1, 1, 1, 0, -1]
        direction = ["u", "ur", "r", "dr", "d", "dl", "l", "ul"]
        for r in r_domain:
            for c in c_domain:
                b_spot = self.get_board_spot(r, c)
                for z in zip(r_list, c_list, direction):
                    l_spot = self.get_board_spot(r + z[0], c + z[1])
                    if l_spot is not None:
                        b_spot.set_link(z[2], l_spot)

    def get_moves(self) -> list[Board_Spot]:
        move_list: list[Board_Spot] = []
        for r in range(self.row):
            for c in range(self.col):
                spot = self.grid[r][c]
                if spot.move != " ":
                    adj_list = spot.get_empty_adjacent()
                    for m in adj_list:
                        if m not in move_list:
                            move_list.append(m)
        return move_list

    def build_vector_lists(self):
        vec_list = []
        for r in range(self.row):
            h_vec = list()
            for c in range(self.col):
                h_vec.append(self.get_board_spot(r, c))
            vec_list.append(h_vec)
        for c in range(self.col):
            v_vec = list()
            for r in range(self.row):
                v_vec.append(self.get_board_spot(r, c))
            vec_list.append(v_vec)
        for r in range(self.row):
            for_slash_vec = list()
            spot = self.get_board_spot(r, 0)
            while spot is not None:
                for_slash_vec.append(spot)
                spot = spot.get_link("dr")
            vec_list.append(for_slash_vec)
        for c in range(1, self.col):
            for_slash_vec = list()
            spot = self.get_board_spot(0, c)
            while spot is not None:
                for_slash_vec.append(spot)
                spot = spot.get_link("dr")
            vec_list.append(for_slash_vec)
        for c in range(0, self.col):
            back_slash_vec = list()
            spot = self.get_board_spot(0, c)
            while spot is not None:
                back_slash_vec.append(spot)
                spot = spot.get_link("dl")
            vec_list.append(back_slash_vec)
        for r in range(1, self.row):
            back_slash_vec = list()
            spot = self.get_board_spot(r, self.col - 1)
            while spot is not None:
                back_slash_vec.append(spot)
                spot = spot.get_link("dl")
            vec_list.append(back_slash_vec)
        return vec_list

    def print_vec_list(self):
        for vec in self.vec_lists:
            print(vec)

    def get_board_spot(self, r: int, c: int):
        key = f"r:{r}c:{c}"
        if key in self.spot_dict:
            return self.spot_dict[key]
        else:
            return None

    def print_spot_link(self, r, c):
        key = f"r:{r}c:{c}"
        if key in self.spot_dict:
            print(self.spot_dict[key].links)
            return
        else:
            return

    def print_board(self):
        # [x] [x]
        for r in range(self.row + 1):
            row = ""
            for c in range(self.col + 1):
                if r == 0:
                    if c == 0:
                        row += "     "
                    else:
                        row += f"{c - 1}   "
                else:
                    if c == 0:
                        row += f"{r - 1}   "
                    else:
                        row += f"[{self.get_board_spot(r - 1, c - 1).get_move()}] "
            print(row)

# you only have to check the last move not the entire board
    def check_board_win(self):
        for r in range(self.row):
            for c in range(self.col):
                win, move = self.check_spot_win(r, c)
                if win:
                    return True, move
        return False, None

    def check_spot_win(self, r, c):
        b_spot = self.get_board_spot(r, c)
        if b_spot.move == " ":
            return False, None
        for direct in self.direction_pairs:
            if (b_spot.get_streak(direct[0]) + b_spot.get_streak(direct[1]) + 1) == self.streak:
                return True, b_spot.move
        return False, None

    def eval_vec(self, symbol: str, vec: list[Board_Spot], mine: bool = True):
        score = 0
        streak = 0
        open_spot = 0
        for i in range(len(vec) + 1):
            if i == len(vec):  # eval where vec ends, to make sure hanging streaks are still counted
                if 2 <= streak:  # This is where streak breaks
                    if streak > 4:
                        streak = 4
                    score += self.h_dict[(streak, open_spot, mine)]
                break
            if vec[i].move == symbol:
                streak += 1
                if i > 0 and vec[i - 1].move == " ":
                    open_spot += 1
                if i < len(vec) - 1 and vec[i + 1].move == " ":
                    open_spot += 1
            elif 2 <= streak:  # This is where streak breaks
                if streak > 4:
                    streak = 4
                score += self.h_dict[(streak, open_spot, mine)]
                streak = 0
                open_spot = 0
            else:
                streak = 0
                open_spot = 0
        return score

    def eval_board(self, my_symbol, your_symbol) -> int:
        score = 0
        for vec in self.vec_lists:
            score += self.eval_vec(my_symbol, vec, True)
        for vec in self.vec_lists:
            score += self.eval_vec(your_symbol, vec, False)
        return score

    def map_heuristic_scores(self):
        # streak first, then open spot, the True if yours
        h_dict = dict[tuple[int, int, bool], int]()
        h_dict[(2, 0, True)] = 0
        h_dict[(2, 1, True)] = 5
        h_dict[(2, 2, True)] = 20
        h_dict[(3, 0, True)] = 0
        h_dict[(3, 1, True)] = 150
        h_dict[(3, 2, True)] = 200
        h_dict[(4, 0, True)] = 1000
        h_dict[(4, 1, True)] = 1000
        h_dict[(4, 2, True)] = 1000
        h_dict[(5, 0, True)] = 2000

        h_dict[(0, 0, True)] = 0

        h_dict[(2, 0, False)] = 0
        h_dict[(2, 1, False)] = -2
        h_dict[(2, 2, False)] = -15
        h_dict[(3, 0, False)] = 0
        h_dict[(3, 1, False)] = -40
        h_dict[(3, 2, False)] = -80
        h_dict[(4, 0, False)] = -1000
        h_dict[(4, 1, False)] = -1000
        h_dict[(4, 2, False)] = -1000
        h_dict[(5, 0, False)] = -2000

        return h_dict


class Player:
    nodes_generated: int

    def __init__(self, my_symbol: str, your_symbol: str, board: Board, depth: int, manual: bool = False):
        self.my_symbol = my_symbol
        self.your_symbol = your_symbol
        self.board = board
        self.search_depth = depth
        self.manual = manual

    def make_move(self):
        self.nodes_generated = 0
        start_time = time.time()
        score, spot = self.minimax(self.board, 0, True)
        time_elapsed = time.time() - start_time
        print(f"Time elapsed: {time_elapsed}")
        print(f"Nodes expanded: {self.nodes_generated}")
        if spot is None:
            return False
        else:
            spot.move = self.my_symbol
            return True

    def minimax(self, board: Board, depth: int, max: bool) -> (int, Board_Spot):
        class Branch_score:
            def __init__(self, b: Board, s: int, move: Board_Spot):
                self.board = b
                self.score = s
                self.move = move

            def print(self):
                print(self.score)
                self.board.print_board()
        move_list = board.get_moves()
        new_max = not max
        board_list: list[Branch_score] = list[Branch_score]()
        if len(move_list) == 0:
            win, move = board.check_board_win()
            if not win:
                return 0, None
            else:
                if move == self.my_symbol:
                    return 1000, None
                else:
                    return -1000, None
        for m in move_list:
            self.nodes_generated += 1
            new_board = board.copy_board()
            if max:
                new_board.grid[m.row][m.col].move = self.my_symbol
                h_score = new_board.eval_board(self.my_symbol, self.your_symbol)
            else:
                new_board.grid[m.row][m.col].move = self.your_symbol
                h_score = new_board.eval_board(self.my_symbol, self.your_symbol)
            branch = Branch_score(new_board, h_score, m)
            board_list.append(branch)
        if depth != self.search_depth:
            new_depth = depth + 1
            for b in board_list:
                if b.score > 800 or b.score < -800:
                    continue
                b.score, drop = self.minimax(b.board, new_depth, new_max)
        if max:
            board_list.sort(key=lambda x: (-x.score, x.move.col, x.move.row))
        else:
            board_list.sort(key=lambda x: (x.score, x.move.col, x.move.row))
        return board_list[0].score, board_list[0].move


if __name__ == '__main__':
    board = Board(5, 6, 4)
    player1 = Player('x', 'o', board, 1)
    player2 = Player('o', 'x', board, 3)
    board.get_board_spot(2, 3).set_move('x')
    board.get_board_spot(2, 2).set_move('o')
    current = player1
    save = player2
    print("START, move 1 and move 2")
    board.print_board()
    print()
    winner = False
    win_move = " "
    move_number = 3
    while not winner:
        print(f"Move number: {move_number}")
        move_number += 1
        if not current.make_move():
            winner, win_move = board.check_board_win()
            if winner:
                print(f"Player {win_move} wins!")
            else:
                print(f"Game ended in a tie")
            break
        current, save = save, current
        board.print_board()
        print()
        winner, win_move = board.check_board_win()
        if winner:
            print(f"Player {win_move} wins!")
