
#include <md_doc.hpp>
#include <md_parser.hpp>

#include <md4c.h>

#include <chrono>
#include <iostream>
#include <vector>

#include <QFile>


struct DATA {
	QSharedPointer< MD::Document > * doc;
	std::vector< MD::Item* > elems;
};


int enter_block( MD_BLOCKTYPE type, void * data, void * doc )
{
	auto d = reinterpret_cast< DATA* >( doc );

	QSharedPointer< MD::Item > item;

	switch( type )
	{
		case MD_BLOCK_H :
		{
			item.reset( new MD::Heading );
		}
			break;

		case MD_BLOCK_CODE :
		{
			item.reset( new MD::Code( QStringLiteral( "" ), false ) );
		}
			break;

		case MD_BLOCK_P :
		{
			item.reset( new MD::Paragraph );
		}
			break;

		case MD_BLOCK_QUOTE :
		{
			item.reset( new MD::Blockquote );
		}
			break;

		case MD_BLOCK_UL :
		case MD_BLOCK_OL :
		{
			item.reset( new MD::List );
		}
			break;

		case MD_BLOCK_LI :
		{
			item.reset( new MD::ListItem );
		}
			break;

		case MD_BLOCK_TABLE :
		{
			item.reset( new MD::Table );
		}
			break;

		case MD_BLOCK_THEAD :
		case MD_BLOCK_TR :
		{
			item.reset( new MD::TableRow );
		}
			break;

		case MD_BLOCK_TH :
		case MD_BLOCK_TD :
		{
			item.reset( new MD::TableCell );
		}
			break;

		default :
			break;
	}

	if( item )
	{
		if( d->elems.empty() )
			(*d->doc)->appendItem( item );
		else
		{
			switch( d->elems.back()->type() )
			{
				case MD::ItemType::Paragraph :
				case MD::ItemType::Blockquote :
				case MD::ItemType::ListItem :
				case MD::ItemType::List :
				{
					auto * b = static_cast< MD::Block* > ( d->elems.back() );

					b->appendItem( item );
				}
					break;

				case MD::ItemType::Table :
				{
					auto * t = static_cast< MD::Table* > ( d->elems.back() );

					t->appendRow( qSharedPointerCast< MD::TableRow > ( item ) );
				}
					break;

				case MD::ItemType::TableRow :
				{
					auto * t = static_cast< MD::TableRow* > ( d->elems.back() );

					t->appendCell( qSharedPointerCast< MD::TableCell > ( item ) );
				}
					break;

				default :
					(*d->doc)->appendItem( item );
					break;
			}
		}

		d->elems.push_back( item.data() );
	}

	return 0;
}

int leave_block( MD_BLOCKTYPE type, void * data, void * doc )
{
	auto d = reinterpret_cast< DATA* >( doc );

	switch( type )
	{
		case MD_BLOCK_H :
		case MD_BLOCK_CODE :
		case MD_BLOCK_P :
		case MD_BLOCK_QUOTE :
		case MD_BLOCK_UL :
		case MD_BLOCK_OL :
		case MD_BLOCK_LI :
		case MD_BLOCK_TABLE :
		case MD_BLOCK_THEAD :
		case MD_BLOCK_TR :
		case MD_BLOCK_TH :
		case MD_BLOCK_TD :
			d->elems.pop_back();
			break;

		default :
			break;
	}

	return 0;
}

int enter_span( MD_SPANTYPE type, void * data, void * doc )
{
	auto d = reinterpret_cast< DATA* >( doc );

	switch( type )
	{
		case MD_SPAN_CODE :
		{
			QSharedPointer< MD::Item > item( new MD::Code( QStringLiteral( "" ), true ) );

			if( d->elems.empty() )
				(*d->doc)->appendItem( item );
			else
			{
				switch( d->elems.back()->type() )
				{
					case MD::ItemType::Paragraph :
					case MD::ItemType::Blockquote :
					case MD::ItemType::ListItem :
					case MD::ItemType::List :
					{
						auto * b = static_cast< MD::Block* > ( d->elems.back() );

						b->appendItem( item );
					}
						break;

					default :
						break;
				}
			}
		}
			break;

		default :
		{
			QSharedPointer< MD::Item > item( new MD::Text );

			if( d->elems.empty() )
				(*d->doc)->appendItem( item );
			else
			{
				switch( d->elems.back()->type() )
				{
					case MD::ItemType::Paragraph :
					case MD::ItemType::Blockquote :
					case MD::ItemType::ListItem :
					case MD::ItemType::List :
					{
						auto * b = static_cast< MD::Block* > ( d->elems.back() );

						b->appendItem( item );
					}
						break;

					default :
						break;
				}
			}
		}
			break;
	}

	return 0;
}

int leave_span( MD_SPANTYPE type, void * data, void * doc )
{
	auto d = reinterpret_cast< DATA* >( doc );

	switch( type )
	{
		default :
			break;
	}

	return 0;
}

int text( MD_TEXTTYPE type, const MD_CHAR * data, MD_SIZE size, void * doc )
{
	auto d = reinterpret_cast< DATA* >( doc );

	QByteArray buf( data, size );
	QString txt( buf );

	switch( d->elems.back()->type() )
	{
		case MD::ItemType::Code :
		{
			auto * c = static_cast< MD::Code* > ( d->elems.back() );
			c->setText( txt );
		}
			break;

		case MD::ItemType::Paragraph :
		{
			auto * p = static_cast< MD::Paragraph* > ( d->elems.back() );
			QSharedPointer< MD::Text > t( new MD::Text );
			t->setText( txt );
			p->appendItem( t );
		}
			break;

		case MD::ItemType::Heading :
		{
			auto * h = static_cast< MD::Heading* > ( d->elems.back() );
			h->setText( txt );
		}
			break;

		default :
			break;
	}

	return 0;
}

int main( int argc, char ** argv )
{
	// md-pdf
	{
		const auto start = std::chrono::high_resolution_clock::now();

		try {
			MD::Parser parser;

			const auto doc = parser.parse( QStringLiteral( "complex.md" ), false );
		}
		catch( const MD::ParserException & x )
		{
			std::cout << "md-pdf parsing failed: " << x.reason().toStdString() << std::endl;
		}

		const auto end = std::chrono::high_resolution_clock::now();

		const auto d = std::chrono::duration_cast< std::chrono::microseconds > ( end - start );

		std::cout << "md-pdf parsing: " << d.count() << " us" << std::endl;
	}

	//md4c
	{
		const auto start = std::chrono::high_resolution_clock::now();

		QSharedPointer< MD::Document > doc( new MD::Document );

		QFile file( QStringLiteral( "complex.md" ) );

		if( file.open( QIODevice::ReadOnly ) )
		{
			const auto md = file.readAll();

			file.close();

			DATA data;
			data.doc = &doc;

			MD_PARSER parser;
			parser.abi_version = 0;
			parser.flags = MD_FLAG_TABLES;
			parser.enter_block = enter_block;
			parser.leave_block = leave_block;
			parser.enter_span = enter_span;
			parser.leave_span = leave_span;
			parser.text = text;
			parser.debug_log = nullptr;
			parser.syntax = nullptr;

			if( md_parse( md.constData(), md.size(), &parser, &data ) != 0 )
				std::cout << "failed to parse input with md4c" << std::endl;
			else if( data.elems.size() != 0 )
				std::cout << "parsing done. not finished blocks count: " << data.elems.size() << std::endl;
		}
		else
			std::cout << "failed to open input.md" << std::endl;

		const auto end = std::chrono::high_resolution_clock::now();

		const auto d = std::chrono::duration_cast< std::chrono::microseconds > ( end - start );

		std::cout << "md4c parsing: " << d.count() << " us" << std::endl;
	}

	return 0;
}
