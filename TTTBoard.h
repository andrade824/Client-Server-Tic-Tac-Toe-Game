//
// Created by devon on 5/8/15.
//

#ifndef MIDTERM_TICTACTOE_TTTBOARD_H
#define MIDTERM_TICTACTOE_TTTBOARD_H

#define MAX_ROWS 3
#define MAX_COLS 3

enum CellType {EXXES, OHS, BLANK};

class TTTBoard
{
public:
    // Constructor which type this player is
    TTTBoard();

    // The current player makes a move
    void PlayerMakeMove(int row, int col);

    // The other player makes a move
    void OtherMakeMove(int row, int col);

    // Check if the player won
    bool IsWon() const;

    // Check if the players tied
    bool IsDraw() const;

    // Check if the player lost
    bool IsLost() const;

    // Print out the board
    void DrawBoard() const;

    // Setter for the type
    void SetType(CellType type);

    // Returns whether that cell is a blank
    bool IsBlank(int row, int col) const;

private:
    // Check if a certain cell type won
    bool TypeIsWon(CellType type) const;

    // Return back the character for a certain cell
    char PrintCell(CellType cell) const;

private:
    CellType m_type;
    CellType m_board[MAX_ROWS][MAX_COLS];
};


#endif //MIDTERM_TICTACTOE_TTTBOARD_H
