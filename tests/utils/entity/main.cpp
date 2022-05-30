
#include <fstream>
#include <string>
#include <iostream>
#include <vector>


std::string codeToString( const std::string & code )
{
	size_t size = code.length();

	std::string res;

	size_t i = 2;

	bool first = true;

	while( size )
	{
		const auto c = code.substr( i, 4  );
		size -= 6;
		i += 6;

		if( first )
		{
			first = false;
			res += "QString( QChar( 0x" + c + " ) )";
		}
		else
		{
			res += " + QChar( 0x" + c + " )";
		}
	}

	return res;
}


int main()
{
	try {
		std::ifstream in( "entities.json" );

		if( in.good() )
		{
			std::ofstream out( "entities.hpp" );

			out << "namespace MD {\n\n";
			out << "static const std::map< QString, QString > c_entityMap = {\n";

			bool first = true;

			for( std::string line; std::getline( in, line ); )
			{
				const auto firstQuote = line.find( "\"" );

				if( firstQuote != std::string::npos )
				{
					const auto secondQuote = line.find( "\"", firstQuote + 1 );

					const auto name = line.substr( firstQuote + 1, secondQuote - firstQuote - 1 );

					const auto lastQuote = line.rfind( "\"" );

					const auto prevLastQuote = line.rfind( "\"", lastQuote - 1 );

					const auto code = line.substr( prevLastQuote + 1, lastQuote - prevLastQuote - 1 );

					if( out.good() )
					{
						if( !first )
							out << ",\n";

						out << "\t{ QStringLiteral( \"" << name << "\" ), " << codeToString( code ) << " }";
					}
					else
					{
						std::cout << "Unable to create file \"entities.hpp\"\n" << std::endl;

						return -1;
					}

					first = false;
				}
			}

			out << "\n}\n\n}\n";
		}
		else
		{
			std::cout << "Unable to open file \"entities.json\"" << std::endl;

			return -1;
		}
	}
	catch( const std::logic_error & x )
	{
		std::cout << x.what() << std::endl;

		return -1;
	}
}