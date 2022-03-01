# Why it's slow?

To simplify coding of Markdown parser I use `QRegularExpression` and `QString::simplified()`,
what is very expensive. That is why my parser >4 times slower than `md4c`. Look, for example,
at the following code:

```cpp
static const QRegularExpression olr( QStringLiteral( "^\\d+\\.\\s+.*" ) );

const auto match = olr.match( QStringLiteral( "123.   abc" ) );
```

Simple check for ordered list eats 90-150 microseonds. Simple rewrite this functionality
with simple function will eat 0 microseconds.

```cpp
bool isOrderedList( const QString & s )
{
    qsizetype p = 0;

    for( ; p < s.size(); ++p )
    {
        if( !s[ p ].isSpace() )
            break;
    }

    qsizetype dp = p;

    for( ; p < s.size(); )
    {
        if( s[ p ].isDigit() )
            ++p;
        else
        break;
    }

    if( dp != p && p < s.size() )
    {
        if( s[ p ] == QLatin1Char( '.' ) || s[ p ] == QLatin1Char( ')' ) )
        {
            if( ++p < s.size() && s[ p ].isSpace() )
                return true;
        }
    }

    return false;
}
```

I can refuse from using regular expressions and my parser will be competitive with `md4c`.
But is it needed in `md-pdf` converter application, most of the time is used by rendering
to PDF. So let Markdown parsing will be a little slow.

# Why don't I use `md4c` for Markdown parsing?

`md4c` doesn't support footnotes, that `md-pdf` renders. Ok, I don't support tasks lists,
`LaTeX` math expressions yet. But this is because I don't need it yet. If you need something
that is not implemented yet - just create an issue. Thanks.
