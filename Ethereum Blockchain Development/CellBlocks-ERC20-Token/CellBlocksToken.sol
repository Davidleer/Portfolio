
pragma solidity ^0.4.19;

/*
MIT License for burn() function and event

Copyright (c) 2016 Smart Contract Solutions, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


import "./SafeMath.sol";
import "./EIP20Interface.sol";
import "./Ownable.sol";


contract CellBlocksToken is EIP20Interface, Ownable {

    uint256 constant private MAX_UINT256 = 2**256 - 1;
    mapping (address => uint256) public balances;
    mapping (address => mapping (address => uint256)) public allowed;
    /*
    NOTE:
    The following variables are OPTIONAL vanities. One does not have to include them.
    They allow one to customise the token contract & in no way influences the core functionality.
    Some wallets/interfaces might not even bother to look at this information.
    */
    string public name;                   //fancy name: eg Simon Bucks
    uint8 public decimals;                //How many decimals to show.
    string public symbol;                 //An identifier: eg SBX

    function CellBlocksToken() public {
        balances[msg.sender] = (10**26);            // Give the creator all initial tokens
        totalSupply = (10**26);                     // Update total supply
        name = "CellBlocks";                          // Set the name for display purposes
        decimals = 18;                                // Amount of decimals for display purposes
        symbol = "CLBK";                               // Set the symbol for display purposes
    }

    //as long as supply > 10**26 and timestamp is after 6/20/18 12:01 am MST, 
    //transfer will call halfPercent() and burn() to burn 0.5% of each transaction 
    function transfer(address _to, uint256 _value) public returns (bool success) {
        require(balances[msg.sender] >= _value);
        if (totalSupply > 33*(10**24) && block.timestamp >= 1529474460) {
            uint halfP = halfPercent(_value);
            burn(msg.sender, halfP);
            _value = SafeMath.sub(_value, halfP);
        }
        balances[msg.sender] = SafeMath.sub(balances[msg.sender], _value);
        balances[_to] = SafeMath.add(balances[_to], _value);
        Transfer(msg.sender, _to, _value);
        return true;
    }

    //as long as supply > 10**26 and timestamp is after 6/20/18 12:01 am MST, 
    //transferFrom will call halfPercent() and burn() to burn 0.5% of each transaction
    function transferFrom(address _from, address _to, uint256 _value) public returns (bool success) {
        uint256 allowance = allowed[_from][msg.sender];
        require(balances[_from] >= _value && allowance >= _value);
        if (totalSupply > 33*(10**24) && block.timestamp >= 1529474460) {
            uint halfP = halfPercent(_value);
            burn(_from, halfP);
            _value = SafeMath.sub(_value, halfP);
        }
        balances[_to] = SafeMath.add(balances[_to], _value);
        balances[_from] = SafeMath.sub(balances[_from], _value);
        if (allowance < MAX_UINT256) {
            allowed[_from][msg.sender] = SafeMath.sub(allowed[_from][msg.sender], _value);
        }
        Transfer(_from, _to, _value);
        return true;
    }

    function balanceOf(address _owner) public view returns (uint256 balance) {
        return balances[_owner];
    }

    function approve(address _spender, uint256 _value) public returns (bool success) {
        allowed[msg.sender][_spender] = _value;
        Approval(msg.sender, _spender, _value);
        return true;
    }

    function allowance(address _owner, address _spender) public view returns (uint256 remaining) {
        return allowed[_owner][_spender];
    }   

    /// @notice returns uint representing 0.5% of _value
    /// @param _value amount to calculate 0.5% of
    /// @return uint representing 0.5% of _value
    function halfPercent(uint _value) private pure returns(uint amount) {
        if (_value > 0) {
            // caution, check safe-to-multiply here
            uint temp = SafeMath.mul(_value, 5);
            amount = SafeMath.div(temp, 1000);

            if (amount == 0) {
                amount = 1;
            }
        }   
        else {
            amount = 0;
        }
        return;
    }

    /// @notice burns _value of tokens from address burner
    /// @param burner The address to burn the tokens from 
    /// @param _value The amount of tokens to be burnt
    function burn(address burner, uint256 _value) public {
        require(_value <= balances[burner]);
        // no need to require value <= totalSupply, since that would imply the
        // sender's balance is greater than the totalSupply, which *should* be an assertion failure
        if (_value > 0) {
            balances[burner] = SafeMath.sub(balances[burner], _value);
            totalSupply = SafeMath.sub(totalSupply, _value);
            Burn(burner, _value);
            Transfer(burner, address(0), _value);
        }
    }

    event Burn(address indexed burner, uint256 value);
}


